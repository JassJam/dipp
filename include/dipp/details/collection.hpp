#pragma once

#include "storage.hpp"

namespace dipp
{
    template<service_policy_type StoragePolicyTy> class service_collection
    {
        template<service_policy_type, instance_policy_type, instance_policy_type> friend class service_provider;

    public:
        template<base_injected_type InjectableTy> void add(typename InjectableTy::descriptor_type descriptor)
        {
            m_Storage.add_service<typename InjectableTy::descriptor_type, InjectableTy::key>(std::move(descriptor));
        }

        template<base_injected_type InjectableTy> void add()
        {
            m_Storage.add_service<typename InjectableTy::descriptor_type, InjectableTy::key>();
        }

        template<service_descriptor_type DescTy, string_literal key = string_literal<0>{}> void add(DescTy descriptor)
        {
            m_Storage.add_service<DescTy, key>(std::move(descriptor));
        }

        template<service_descriptor_type DescTy, string_literal key = string_literal<0>{}> void add()
        {
            m_Storage.add_service<DescTy, key>();
        }

    private:
        service_storage<StoragePolicyTy> m_Storage;
    };

    using default_service_collection = service_collection<default_service_policy>;
} // namespace dipp