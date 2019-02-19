// Copyright (C) 2018 Brick
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "PatchBuilder.h"
#include "BinaryViewAssociatedDataStore.h"

#include "DataBufferAdapter.h"

#include <unordered_map>
#include <mutex>

#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <bitsery/flexible.h>
#include <bitsery/flexible/unordered_map.h>

static const std::string PATCH_METADATA_KEY = "OBFU_PATCHES";
static const std::string PATCH_METADATA_VERSION = "0.0.0";

namespace PatchBuilder
{
    struct PatchCollection
    {
        std::unordered_map<uintptr_t, Patch> m_Patches;
        mutable std::mutex m_Mutex;

        void AddPatch(uintptr_t address, Patch patch);
        const Patch* GetPatch(uintptr_t address) const;

        void Save(BinaryView& view);
        void Load(BinaryView& view);
    };

    BinaryViewAssociatedDataStore<PatchCollection> PatchStore;

    PatchCollection* GetPatchCollection(BNBinaryView* view)
    {
        if (PatchCollection* patches = PatchStore.Get(view))
        {
            return patches;
        }

        std::unique_ptr<PatchCollection> patches(new PatchCollection());

        Ref<BinaryView> ref_view = new BinaryView(view);

        patches->Load(*ref_view);

        PatchStore.Set(view, std::move(patches));

        return PatchStore.Get(view);
    }

    void AddPatch(BinaryView& view, uintptr_t address, Patch patch)
    {
        PatchCollection* patches = GetPatchCollection(view.m_object);

        patches->AddPatch(address, std::move(patch));
    }

    const Patch* GetPatch(LowLevelILFunction& il, uintptr_t address)
    {
        BNFunction* function = BNGetLowLevelILOwnerFunction(il.m_object);

        if (function == nullptr)
        {
            return nullptr;
        }

        BNBinaryView* view = BNGetFunctionData(function);
        BNFreeFunction(function);

        if (view == nullptr)
        {
            return nullptr;
        }

        const PatchCollection* patches = GetPatchCollection(view);
        BNFreeBinaryView(view);

        if (patches == nullptr)
        {
            return nullptr;
        }

        return patches->GetPatch(address);
    }

    void LoadPatches(BinaryView & view)
    {
        PatchCollection* patches = GetPatchCollection(view.m_object);

        patches->Load(view);
    }

    void SavePatches(BinaryView & view)
    {
        PatchCollection* patches = GetPatchCollection(view.m_object);

        patches->Save(view);
    }

    void PatchCollection::AddPatch(uintptr_t address, Patch patch)
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_Patches.emplace(address, std::move(patch));
    }

    const Patch* PatchCollection::GetPatch(uintptr_t address) const
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        auto find = m_Patches.find(address);

        if (find != m_Patches.end())
        {
            return &find->second;
        }

        return nullptr;
    }

    void PatchCollection::Save(BinaryView& view)
    {
        DataBuffer db;

        if (bitsery::quickSerialization<OutputDataBufferAdapater>(db, m_Patches))
        {
            if (db.ZlibCompress(db))
            {
                std::vector<uint8_t> raw(
                    static_cast<const uint8_t*>(db.GetData()),
                    static_cast<const uint8_t*>(db.GetData()) + db.GetLength()
                );

                Ref<Metadata> patches = new Metadata
                ({
                    { "version", new Metadata(PATCH_METADATA_VERSION) },
                    { "data", new Metadata(raw) }
                });

                view.StoreMetadata(PATCH_METADATA_KEY, patches);
            }
            else
            {
                BinjaLog(ErrorLog, "Failed to compress patch data for {0}", view.GetFile()->GetFilename());
            }
        }
        else
        {
            BinjaLog(ErrorLog, "Failed to serialize patch data for {0}", view.GetFile()->GetFilename());
        }
    }

    void PatchCollection::Load(BinaryView& view)
    {
        m_Patches.clear();

        Ref<Metadata> patches = view.QueryMetadata(PATCH_METADATA_KEY);

        if (patches && patches->IsKeyValueStore())
        {
            std::map<std::string, Ref<Metadata>> data = patches->GetKeyValueStore();

            if (data.at("version")->GetString() == PATCH_METADATA_VERSION)
            {
                std::vector<uint8_t> raw = data.at("data")->GetRaw();
                DataBuffer db(raw.data(), raw.size());

                if (db.ZlibDecompress(db))
                {
                    if (bitsery::quickDeserialization<InputDataBufferAdapater>({ db }, m_Patches).first == bitsery::ReaderError::NoError)
                    {
                        BinjaLog(InfoLog, "Successfully loaded patch data for {0}", view.GetFile()->GetFilename());
                    }
                    else
                    {
                        BinjaLog(ErrorLog, "Failed to deserialize patch data for {0}", view.GetFile()->GetFilename());
                    }
                }
                else
                {
                    BinjaLog(ErrorLog, "Failed to decompress patch data for {0}", view.GetFile()->GetFilename());
                }
            }
            else
            {
                BinjaLog(ErrorLog, "Outdated or invalid patch data for {0}", view.GetFile()->GetFilename());
            }
        }
    }

    bool Patch::Evaluate(LowLevelILFunction& il) const
    {
        std::vector<size_t> operands;

        for (const Token& token : Tokens)
        {
            switch (token.Type)
            {
                case TokenType::Instruction:
                {
                    BNLowLevelILOperation operation = static_cast<BNLowLevelILOperation>(token.Value);

                    if (operands.size() < 3)
                    {
                        BinjaLog(ErrorLog, "Missing Instruction Operands (expected 3, got {0})", operands.size());

                        return false;
                    }

                    size_t size = operands.back();
                    operands.pop_back();

                    uint32_t flags = static_cast<uint32_t>(operands.back());
                    operands.pop_back();

                    size_t operand_count = operands.back();
                    operands.pop_back();

                    size_t expected_operand_count = LowLevelILInstruction::operationOperandUsage.at(operation).size();

                    if (expected_operand_count != operand_count)
                    {
                        BinjaLog(ErrorLog, "Mismatched operand count (expected {0}, got {1})", expected_operand_count, operand_count);

                        return false;
                    }

                    if (operands.size() < operand_count)
                    {
                        BinjaLog(ErrorLog, "Missing Exprs (expected {0}, got {1})", operand_count, operands.size());

                        return false;
                    }

                    size_t exprs[4]{};
                    auto expr_iter = operands.end() - operand_count;
                    std::copy_n(expr_iter, operand_count, exprs);
                    operands.erase(expr_iter, operands.end());

                    ExprId expr = il.AddExpr(operation, size, flags,
                        static_cast<ExprId>(exprs[0]),
                        static_cast<ExprId>(exprs[1]),
                        static_cast<ExprId>(exprs[2]),
                        static_cast<ExprId>(exprs[3])
                    );

                    operands.push_back(static_cast<size_t>(expr));

                } break;

                case TokenType::Operand:
                {
                    operands.push_back(token.Value);
                } break;

                default:
                {
                    BinjaLog(ErrorLog, "Bad Token: {0} - {1}", static_cast<int>(token.Type), token.Value);

                    return false;
                } break;
            }
        }

        for (size_t expr : operands)
        {
            il.AddInstruction(static_cast<ExprId>(expr));
        }

        return true;
    }
}
