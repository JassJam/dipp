#pragma once

#include "details/descriptors.hpp"
#include "details/collection.hpp"
#include "details/provider.hpp"
#include "details/injected.hpp"
#include "details/apply.hpp"

namespace dipp
{
    using details::base_service_collection;
    using details::base_service_getter;
    using details::base_service_provider;
    using details::base_service_scope;

    using details::service_collection;
    using details::service_getter;
    using details::service_lifetime;
    using details::service_provider;
    using details::service_scope;

    using details::apply;
    using details::key;
    using details::make_any;
    using details::make_error;
    using details::make_result;

    using details::base_service_descriptor;
    using details::dependency;
    using details::functor_service_descriptor;
    using details::local_service_descriptor;
    using details::shared_service_descriptor;
    using details::unique_service_descriptor;

    using details::base_injected;
    using details::injected;
    using details::injected_functor;
    using details::injected_ref;
    using details::injected_shared;
    using details::injected_unique;
    using details::result;

    using details::base_error;
    using details::incompatible_service_descriptor;
    using details::mismatched_service_type;
    using details::service_not_found;
}