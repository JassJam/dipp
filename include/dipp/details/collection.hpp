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
            m_Storage.template add_service<typename InjectableTy::descriptor_type, InjectableTy::key>(
                std::move(descriptor));
        }

        template<base_injected_type InjectableTy> void add()
        {
            m_Storage.template add_service<typename InjectableTy::descriptor_type, InjectableTy::key>();
        }

        template<service_descriptor_type DescTy, string_hash Key = string_hash{}> void add(DescTy descriptor)
        {
            m_Storage.template add_service<DescTy, Key>(std::move(descriptor));
        }

        template<service_descriptor_type DescTy, string_hash Key = string_hash{}> void add()
        {
            m_Storage.template add_service<DescTy, Key>();
        }

    private:
        service_storage<StoragePolicyTy> m_Storage;
    };

    using default_service_collection = service_collection<default_service_policy>;
} // namespace dipp