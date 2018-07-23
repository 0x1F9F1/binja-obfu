#include "ObfuArchitectureHook.h"

#include "BinaryViewAssociatedDataStore.h"

#include <mutex>
#include <deque>

namespace PatchBuilder
{
    BinaryViewAssociatedDataStore<PatchCollection> PatchStore;

    Patch* GetPatch(LowLevelILFunction& il, uintptr_t address)
    {
        auto function = BNGetLowLevelILOwnerFunction(il.m_object);

        if (function == nullptr)
        {
            return nullptr;
        }

        auto view = BNGetFunctionData(function);

        if (view == nullptr)
        {
            return nullptr;
        }

        auto view_data = PatchStore.Get(view);

        if (view_data == nullptr)
        {
            return nullptr;
        }

        return view_data->GetPatch(address);
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

    Patch* PatchCollection::GetPatch(uintptr_t address)
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        auto find = m_Patches.find(address);

        if (find != m_Patches.end())
        {
            return &find->second;
        }

        return nullptr;
    }

    bool Patch::Evaluate(LowLevelILFunction& il)
    {
        std::deque<size_t> values;

        for (const Token& token : Tokens)
        {
            switch (token.Type)
            {
                case TokenType::Instruction:
                {
                    BNLowLevelILOperation operation = static_cast<BNLowLevelILOperation>(token.Value);

                    if (values.size() < 3)
                    {
                        LogError("Missing Instruction Operands (expected 3, got %zu)", values.size());

                        return false;
                    }

                    size_t size = values.back();
                    values.pop_back();

                    uint32_t flags = static_cast<uint32_t>(values.back());
                    values.pop_back();

                    size_t expr_count = values.back();
                    values.pop_back();

                    if (values.size() < expr_count)
                    {
                        LogError("Missing Exprs (expected %zu, got %zu)", expr_count, values.size());

                        return false;
                    }

                    ExprId exprs[4]{};

                    for (size_t i = expr_count; i--;)
                    {
                        exprs[i] = values.back();

                        values.pop_back();
                    }

                    ExprId expr = il.AddExprWithLocation(operation, ILSourceLocation(), size, flags,
                        exprs[0],
                        exprs[1],
                        exprs[2],
                        exprs[3]
                    );

                    // LogInfo("Operation %zX at expr %zu (size %zu, flags %u, %zu operands - %zu %zu %zu %zu)", operation, expr, size, flags, expr_count, exprs[0], exprs[1], exprs[2], exprs[3]);

                    values.push_back(static_cast<size_t>(expr));

                } break;

                case TokenType::Operand:
                {
                    values.push_back(token.Value);
                } break;

                default:
                {
                    LogError("Bad Token: %i - %zX", token.Type, token.Value);

                    return false;
                } break;
            }
        }

        for (; !values.empty(); values.pop_front())
        {
            ExprId expr = (ExprId)values.front();

            // LogInfo("Adding expr %zu", expr);

            il.AddInstruction(expr);
        }

        return true;
    }
}

bool ObfuArchitectureHook::GetInstructionLowLevelIL(const uint8_t* data, uint64_t addr, size_t& len, LowLevelILFunction& il)
{
    PatchBuilder::Patch* patch = PatchBuilder::GetPatch(il, addr);

    if (patch && patch->Evaluate(il))
    {
        len = patch->Size;

        return true;
    }

    return ArchitectureHook::GetInstructionLowLevelIL(data, addr, len, il);
}
