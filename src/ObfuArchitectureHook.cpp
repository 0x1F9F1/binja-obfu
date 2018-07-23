#include "ObfuArchitectureHook.h"

#include "BinaryViewAssociatedDataStore.h"

#include <mutex>
#include <deque>

namespace PatchBuilder
{
    BinaryViewAssociatedDataStore<PatchCollection> PatchStore;

    const Patch* GetPatch(LowLevelILFunction& il, uintptr_t address)
    {
        BNFunction* function = BNGetLowLevelILOwnerFunction(il.m_object);

        if (function == nullptr)
        {
            return nullptr;
        }

        BNBinaryView* view = BNGetFunctionData(function);

        if (view == nullptr)
        {
            return nullptr;
        }

        const PatchCollection* patches = PatchStore.Get(view);

        if (patches == nullptr)
        {
            return nullptr;
        }

        return patches->GetPatch(address);
    }

    void AddPatch(BinaryView& view, uintptr_t address, Patch patch)
    {
        PatchStore.GetOrCreate(view.m_object)->AddPatch(address, std::move(patch));
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
                        LogError("Missing Instruction Operands (expected 3, got %zu)", operands.size());

                        return false;
                    }

                    size_t size = operands.back();
                    operands.pop_back();

                    uint32_t flags = static_cast<uint32_t>(operands.back());
                    operands.pop_back();

                    size_t operand_count = operands.back();
                    operands.pop_back();

                    if (operands.size() < operand_count)
                    {
                        LogError("Missing Exprs (expected %zu, got %zu)", operand_count, operands.size());

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
                    LogError("Bad Token: %i - %zX", token.Type, token.Value);

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

bool ObfuArchitectureHook::GetInstructionLowLevelIL(const uint8_t* data, uint64_t addr, size_t& len, LowLevelILFunction& il)
{
    const PatchBuilder::Patch* patch = PatchBuilder::GetPatch(il, addr);

    if (patch && patch->Evaluate(il))
    {
        len = patch->Size;

        return true;
    }

    return ArchitectureHook::GetInstructionLowLevelIL(data, addr, len, il);
}
