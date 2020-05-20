/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2020 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <openssl/crypto.h>
#include <asio/version.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <http_parser.h>

#include <version.hxx>
#include <cluster.hxx>
#include <operations.hxx>

#include <ruby.h>

#if !defined(RB_METHOD_DEFINITION_DECL)
#define VALUE_FUNC(f) reinterpret_cast<VALUE (*)(ANYARGS)>(f)
#define INT_FUNC(f) reinterpret_cast<int (*)(ANYARGS)>(f)
#else
#define VALUE_FUNC(f) (f)
#define INT_FUNC(f) (f)
#endif

static void
init_versions(VALUE mCouchbase)
{
    VALUE cb_Version{};
    if (rb_const_defined(mCouchbase, rb_intern("VERSION")) != 0) {
        cb_Version = rb_const_get(mCouchbase, rb_intern("VERSION"));
    } else {
        cb_Version = rb_hash_new();
        rb_const_set(mCouchbase, rb_intern("VERSION"), cb_Version);
    }
#define VERSION_SPLIT_(VER) (VER) / 100000, (VER) / 100 % 1000, (VER) % 100

    std::string ver;
    ver = fmt::format("{}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    rb_hash_aset(cb_Version, rb_id2sym(rb_intern("spdlog")), rb_str_freeze(rb_str_new(ver.c_str(), static_cast<long>(ver.size()))));
    ver = fmt::format("{}.{}.{}", VERSION_SPLIT_(ASIO_VERSION));
    rb_hash_aset(cb_Version, rb_id2sym(rb_intern("asio")), rb_str_freeze(rb_str_new(ver.c_str(), static_cast<long>(ver.size()))));
    ver = fmt::format("{}.{}.{}", HTTP_PARSER_VERSION_MAJOR, HTTP_PARSER_VERSION_MINOR, HTTP_PARSER_VERSION_PATCH);
    rb_hash_aset(cb_Version, rb_id2sym(rb_intern("http_parser")), rb_str_freeze(rb_str_new(ver.c_str(), static_cast<long>(ver.size()))));
    ver = fmt::format("{}.{}.{}", BACKEND_VERSION_MAJOR, BACKEND_VERSION_MINOR, BACKEND_VERSION_PATCH);
    rb_hash_aset(cb_Version, rb_id2sym(rb_intern("backend")), rb_str_freeze(rb_str_new(ver.c_str(), static_cast<long>(ver.size()))));
    rb_hash_aset(cb_Version, rb_id2sym(rb_intern("openssl_headers")), rb_str_freeze(rb_str_new_cstr(OPENSSL_VERSION_TEXT)));
#if defined(OPENSSL_VERSION)
    rb_hash_aset(cb_Version, rb_id2sym(rb_intern("openssl_runtime")), rb_str_freeze(rb_str_new_cstr(OpenSSL_version(OPENSSL_VERSION))));
#elif defined(SSLEAY_VERSION)
    rb_hash_aset(cb_Version, rb_id2sym(rb_intern("openssl_runtime")), rb_str_freeze(rb_str_new_cstr(SSLeay_version(SSLEAY_VERSION))));
#endif

#undef VERSION_SPLIT_
}

struct cb_backend_data {
    std::unique_ptr<asio::io_context> ctx;
    std::unique_ptr<couchbase::cluster> cluster;
    std::thread worker;
};

static void
cb__backend_close(cb_backend_data* backend)
{
    if (backend->cluster) {
        backend->cluster.reset(nullptr);
        if (backend->worker.joinable()) {
            backend->worker.join();
        }
        backend->ctx.reset(nullptr);
    }
}

static void
cb_Backend_mark(void* /* ptr */)
{
}

static void
cb_Backend_free(void* ptr)
{
    auto* backend = reinterpret_cast<cb_backend_data*>(ptr);
    cb__backend_close(backend);
    ruby_xfree(backend);
}

static size_t
cb_Backend_memsize(const void* ptr)
{
    const auto* backend = reinterpret_cast<const cb_backend_data*>(ptr);
    return sizeof(*backend) + sizeof(*backend->cluster);
}

static const rb_data_type_t cb_backend_type{
    "Couchbase/Backend",
    { cb_Backend_mark,
      cb_Backend_free,
      cb_Backend_memsize,
// only one reserved field when GC.compact implemented
#ifdef T_MOVED
      nullptr,
#endif
      {} },
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
    nullptr,
    nullptr,
    RUBY_TYPED_FREE_IMMEDIATELY,
#endif
};

static VALUE
cb_Backend_allocate(VALUE klass)
{
    cb_backend_data* backend = nullptr;
    VALUE obj = TypedData_Make_Struct(klass, cb_backend_data, &cb_backend_type, backend);
    backend->ctx = std::make_unique<asio::io_context>();
    backend->cluster = std::make_unique<couchbase::cluster>(*backend->ctx);
    backend->worker = std::thread([backend]() { backend->ctx->run(); });
    return obj;
}

static VALUE eBackendError;
static VALUE eAmbiguousTimeout;
static VALUE eAuthenticationFailure;
static VALUE eBucketExists;
static VALUE eBucketNotFlushable;
static VALUE eBucketNotFound;
static VALUE eCasMismatch;
static VALUE eCollectionExists;
static VALUE eCollectionNotFound;
static VALUE eCompilationFailure;
static VALUE eDatasetExists;
static VALUE eDatasetNotFound;
static VALUE eDataverseExists;
static VALUE eDataverseNotFound;
static VALUE eDecodingFailure;
static VALUE eDeltaInvalid;
static VALUE eDesignDocumentNotFound;
static VALUE eDocumentExists;
static VALUE eDocumentIrretrievable;
static VALUE eDocumentLocked;
static VALUE eDocumentNotFound;
static VALUE eDocumentNotJson;
static VALUE eDurabilityAmbiguous;
static VALUE eDurabilityImpossible;
static VALUE eDurabilityLevelNotAvailable;
static VALUE eDurableWriteInProgress;
static VALUE eDurableWriteReCommitInProgress;
static VALUE eEncodingFailure;
static VALUE eFeatureNotAvailable;
static VALUE eGroupNotFound;
static VALUE eIndexExists;
static VALUE eIndexFailure;
static VALUE eIndexNotFound;
static VALUE eInternalServerFailure;
static VALUE eInvalidArgument;
static VALUE eJobQueueFull;
static VALUE eLinkNotFound;
static VALUE eNumberTooBig;
static VALUE eParsingFailure;
static VALUE ePathExists;
static VALUE ePathInvalid;
static VALUE ePathMismatch;
static VALUE ePathNotFound;
static VALUE ePathTooBig;
static VALUE ePathTooDeep;
static VALUE ePlanningFailure;
static VALUE ePreparedStatementFailure;
static VALUE eRequestCanceled;
static VALUE eScopeExists;
static VALUE eScopeNotFound;
static VALUE eServiceNotAvailable;
static VALUE eTemporaryFailure;
static VALUE eUnambiguousTimeout;
static VALUE eUnsupportedOperation;
static VALUE eUserNotFound;
static VALUE eUserExists;
static VALUE eValueInvalid;
static VALUE eValueTooDeep;
static VALUE eValueTooLarge;
static VALUE eViewNotFound;
static VALUE eXattrCannotModifyVirtualAttribute;
static VALUE eXattrInvalidKeyCombo;
static VALUE eXattrUnknownMacro;
static VALUE eXattrUnknownVirtualAttribute;

static void
init_exceptions(VALUE mCouchbase)
{
    eBackendError = rb_define_class_under(mCouchbase, "BackendError", rb_eStandardError);
    eAmbiguousTimeout = rb_define_class_under(mCouchbase, "AmbiguousTimeout", rb_eStandardError);
    eAuthenticationFailure = rb_define_class_under(mCouchbase, "AuthenticationFailure", rb_eStandardError);
    eBucketExists = rb_define_class_under(mCouchbase, "BucketExists", rb_eStandardError);
    eBucketNotFlushable = rb_define_class_under(mCouchbase, "BucketNotFlushable", rb_eStandardError);
    eBucketNotFound = rb_define_class_under(mCouchbase, "BucketNotFound", rb_eStandardError);
    eCasMismatch = rb_define_class_under(mCouchbase, "CasMismatch", rb_eStandardError);
    eCollectionExists = rb_define_class_under(mCouchbase, "CollectionExists", rb_eStandardError);
    eCollectionNotFound = rb_define_class_under(mCouchbase, "CollectionNotFound", rb_eStandardError);
    eCompilationFailure = rb_define_class_under(mCouchbase, "CompilationFailure", rb_eStandardError);
    eDatasetExists = rb_define_class_under(mCouchbase, "DatasetExists", rb_eStandardError);
    eDatasetNotFound = rb_define_class_under(mCouchbase, "DatasetNotFound", rb_eStandardError);
    eDataverseExists = rb_define_class_under(mCouchbase, "DataverseExists", rb_eStandardError);
    eDataverseNotFound = rb_define_class_under(mCouchbase, "DataverseNotFound", rb_eStandardError);
    eDecodingFailure = rb_define_class_under(mCouchbase, "DecodingFailure", rb_eStandardError);
    eDeltaInvalid = rb_define_class_under(mCouchbase, "DeltaInvalid", rb_eStandardError);
    eDesignDocumentNotFound = rb_define_class_under(mCouchbase, "DesignDocumentNotFound", rb_eStandardError);
    eDocumentExists = rb_define_class_under(mCouchbase, "DocumentExists", rb_eStandardError);
    eDocumentIrretrievable = rb_define_class_under(mCouchbase, "DocumentIrretrievable", rb_eStandardError);
    eDocumentLocked = rb_define_class_under(mCouchbase, "DocumentLocked", rb_eStandardError);
    eDocumentNotFound = rb_define_class_under(mCouchbase, "DocumentNotFound", rb_eStandardError);
    eDocumentNotJson = rb_define_class_under(mCouchbase, "DocumentNotJson", rb_eStandardError);
    eDurabilityAmbiguous = rb_define_class_under(mCouchbase, "DurabilityAmbiguous", rb_eStandardError);
    eDurabilityImpossible = rb_define_class_under(mCouchbase, "DurabilityImpossible", rb_eStandardError);
    eDurabilityLevelNotAvailable = rb_define_class_under(mCouchbase, "DurabilityLevelNotAvailable", rb_eStandardError);
    eDurableWriteInProgress = rb_define_class_under(mCouchbase, "DurableWriteInProgress", rb_eStandardError);
    eDurableWriteReCommitInProgress = rb_define_class_under(mCouchbase, "DurableWriteReCommitInProgress", rb_eStandardError);
    eEncodingFailure = rb_define_class_under(mCouchbase, "EncodingFailure", rb_eStandardError);
    eFeatureNotAvailable = rb_define_class_under(mCouchbase, "FeatureNotAvailable", rb_eStandardError);
    eGroupNotFound = rb_define_class_under(mCouchbase, "GroupNotFound", rb_eStandardError);
    eIndexExists = rb_define_class_under(mCouchbase, "IndexExists", rb_eStandardError);
    eIndexFailure = rb_define_class_under(mCouchbase, "IndexFailure", rb_eStandardError);
    eIndexNotFound = rb_define_class_under(mCouchbase, "IndexNotFound", rb_eStandardError);
    eInternalServerFailure = rb_define_class_under(mCouchbase, "InternalServerFailure", rb_eStandardError);
    eInvalidArgument = rb_define_class_under(mCouchbase, "InvalidArgument", rb_eStandardError);
    eJobQueueFull = rb_define_class_under(mCouchbase, "JobQueueFull", rb_eStandardError);
    eLinkNotFound = rb_define_class_under(mCouchbase, "LinkNotFound", rb_eStandardError);
    eNumberTooBig = rb_define_class_under(mCouchbase, "NumberTooBig", rb_eStandardError);
    eParsingFailure = rb_define_class_under(mCouchbase, "ParsingFailure", rb_eStandardError);
    ePathExists = rb_define_class_under(mCouchbase, "PathExists", rb_eStandardError);
    ePathInvalid = rb_define_class_under(mCouchbase, "PathInvalid", rb_eStandardError);
    ePathMismatch = rb_define_class_under(mCouchbase, "PathMismatch", rb_eStandardError);
    ePathNotFound = rb_define_class_under(mCouchbase, "PathNotFound", rb_eStandardError);
    ePathTooBig = rb_define_class_under(mCouchbase, "PathTooBig", rb_eStandardError);
    ePathTooDeep = rb_define_class_under(mCouchbase, "PathTooDeep", rb_eStandardError);
    ePlanningFailure = rb_define_class_under(mCouchbase, "PlanningFailure", rb_eStandardError);
    ePreparedStatementFailure = rb_define_class_under(mCouchbase, "PreparedStatementFailure", rb_eStandardError);
    eRequestCanceled = rb_define_class_under(mCouchbase, "RequestCanceled", rb_eStandardError);
    eScopeExists = rb_define_class_under(mCouchbase, "ScopeExists", rb_eStandardError);
    eScopeNotFound = rb_define_class_under(mCouchbase, "ScopeNotFound", rb_eStandardError);
    eServiceNotAvailable = rb_define_class_under(mCouchbase, "ServiceNotAvailable", rb_eStandardError);
    eTemporaryFailure = rb_define_class_under(mCouchbase, "TemporaryFailure", rb_eStandardError);
    eUnambiguousTimeout = rb_define_class_under(mCouchbase, "UnambiguousTimeout", rb_eStandardError);
    eUnsupportedOperation = rb_define_class_under(mCouchbase, "UnsupportedOperation", rb_eStandardError);
    eUserNotFound = rb_define_class_under(mCouchbase, "UserNotFound", rb_eStandardError);
    eUserExists = rb_define_class_under(mCouchbase, "UserExists", rb_eStandardError);
    eValueInvalid = rb_define_class_under(mCouchbase, "ValueInvalid", rb_eStandardError);
    eValueTooDeep = rb_define_class_under(mCouchbase, "ValueTooDeep", rb_eStandardError);
    eValueTooLarge = rb_define_class_under(mCouchbase, "ValueTooLarge", rb_eStandardError);
    eViewNotFound = rb_define_class_under(mCouchbase, "ViewNotFound", rb_eStandardError);
    eXattrCannotModifyVirtualAttribute = rb_define_class_under(mCouchbase, "XattrCannotModifyVirtualAttribute", rb_eStandardError);
    eXattrInvalidKeyCombo = rb_define_class_under(mCouchbase, "XattrInvalidKeyCombo", rb_eStandardError);
    eXattrUnknownMacro = rb_define_class_under(mCouchbase, "XattrUnknownMacro", rb_eStandardError);
    eXattrUnknownVirtualAttribute = rb_define_class_under(mCouchbase, "XattrUnknownVirtualAttribute", rb_eStandardError);
}

static NORETURN(void cb_raise_error_code(std::error_code ec, const std::string& message))
{
    if (ec.category() == couchbase::error::detail::get_common_category()) {
        switch (static_cast<couchbase::error::common_errc>(ec.value())) {
            case couchbase::error::common_errc::unambiguous_timeout:
                rb_raise(eUnambiguousTimeout, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::ambiguous_timeout:
                rb_raise(eAmbiguousTimeout, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::request_canceled:
                rb_raise(eRequestCanceled, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::invalid_argument:
                rb_raise(eInvalidArgument, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::service_not_available:
                rb_raise(eServiceNotAvailable, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::internal_server_failure:
                rb_raise(eInternalServerFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::authentication_failure:
                rb_raise(eAuthenticationFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::temporary_failure:
                rb_raise(eTemporaryFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::parsing_failure:
                rb_raise(eParsingFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::cas_mismatch:
                rb_raise(eCasMismatch, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::bucket_not_found:
                rb_raise(eBucketNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::scope_not_found:
                rb_raise(eScopeNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::collection_not_found:
                rb_raise(eCollectionNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::unsupported_operation:
                rb_raise(eUnsupportedOperation, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::feature_not_available:
                rb_raise(eFeatureNotAvailable, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::encoding_failure:
                rb_raise(eEncodingFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::decoding_failure:
                rb_raise(eDecodingFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::index_not_found:
                rb_raise(eIndexNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::common_errc::index_exists:
                rb_raise(eIndexExists, "%s: %s", message.c_str(), ec.message().c_str());
        }
    } else if (ec.category() == couchbase::error::detail::get_key_value_category()) {
        switch (static_cast<couchbase::error::key_value_errc>(ec.value())) {
            case couchbase::error::key_value_errc::document_not_found:
                rb_raise(eDocumentNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::document_irretrievable:
                rb_raise(eDocumentIrretrievable, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::document_locked:
                rb_raise(eDocumentLocked, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::value_too_large:
                rb_raise(eValueTooLarge, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::document_exists:
                rb_raise(eDocumentExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::durability_level_not_available:
                rb_raise(eDurabilityLevelNotAvailable, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::durability_impossible:
                rb_raise(eDurabilityImpossible, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::durability_ambiguous:
                rb_raise(eDurabilityAmbiguous, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::durable_write_in_progress:
                rb_raise(eDurableWriteInProgress, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::durable_write_re_commit_in_progress:
                rb_raise(eDurableWriteReCommitInProgress, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::path_not_found:
                rb_raise(ePathNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::path_mismatch:
                rb_raise(ePathMismatch, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::path_invalid:
                rb_raise(ePathInvalid, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::path_too_big:
                rb_raise(ePathTooBig, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::path_too_deep:
                rb_raise(ePathTooDeep, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::value_too_deep:
                rb_raise(eValueTooDeep, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::value_invalid:
                rb_raise(eValueInvalid, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::document_not_json:
                rb_raise(eDocumentNotJson, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::number_too_big:
                rb_raise(eNumberTooBig, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::delta_invalid:
                rb_raise(eDeltaInvalid, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::path_exists:
                rb_raise(ePathExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::xattr_unknown_macro:
                rb_raise(eXattrUnknownMacro, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::xattr_invalid_key_combo:
                rb_raise(eXattrInvalidKeyCombo, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::xattr_unknown_virtual_attribute:
                rb_raise(eXattrUnknownVirtualAttribute, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::key_value_errc::xattr_cannot_modify_virtual_attribute:
                rb_raise(eXattrCannotModifyVirtualAttribute, "%s: %s", message.c_str(), ec.message().c_str());
        }
    } else if (ec.category() == couchbase::error::detail::get_query_category()) {
        switch (static_cast<couchbase::error::query_errc>(ec.value())) {
            case couchbase::error::query_errc::planning_failure:
                rb_raise(ePlanningFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::query_errc::index_failure:
                rb_raise(eIndexFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::query_errc::prepared_statement_failure:
                rb_raise(ePreparedStatementFailure, "%s: %s", message.c_str(), ec.message().c_str());
        }
    } else if (ec.category() == couchbase::error::detail::get_view_category()) {
        switch (static_cast<couchbase::error::view_errc>(ec.value())) {
            case couchbase::error::view_errc::view_not_found:
                rb_raise(eViewNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::view_errc::design_document_not_found:
                rb_raise(eDesignDocumentNotFound, "%s: %s", message.c_str(), ec.message().c_str());
        }
    } else if (ec.category() == couchbase::error::detail::get_analytics_category()) {
        switch (static_cast<couchbase::error::analytics_errc>(ec.value())) {
            case couchbase::error::analytics_errc::compilation_failure:
                rb_raise(eCompilationFailure, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::analytics_errc::job_queue_full:
                rb_raise(eJobQueueFull, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::analytics_errc::dataset_not_found:
                rb_raise(eDatasetNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::analytics_errc::dataverse_not_found:
                rb_raise(eDataverseNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::analytics_errc::dataset_exists:
                rb_raise(eDatasetExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::analytics_errc::dataverse_exists:
                rb_raise(eDataverseExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::analytics_errc::link_not_found:
                rb_raise(eLinkNotFound, "%s: %s", message.c_str(), ec.message().c_str());
        }
    } else if (ec.category() == couchbase::error::detail::get_management_category()) {
        switch (static_cast<couchbase::error::management_errc>(ec.value())) {
            case couchbase::error::management_errc::collection_exists:
                rb_raise(eCollectionExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::management_errc::scope_exists:
                rb_raise(eScopeExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::management_errc::user_not_found:
                rb_raise(eUserNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::management_errc::group_not_found:
                rb_raise(eGroupNotFound, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::management_errc::user_exists:
                rb_raise(eUserExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::management_errc::bucket_exists:
                rb_raise(eBucketExists, "%s: %s", message.c_str(), ec.message().c_str());

            case couchbase::error::management_errc::bucket_not_flushable:
                rb_raise(eBucketNotFlushable, "%s: %s", message.c_str(), ec.message().c_str());
        }
    }

    rb_raise(eBackendError, "%s: %s", message.c_str(), ec.message().c_str());
}

static VALUE
cb_Backend_open(VALUE self, VALUE hostname, VALUE username, VALUE password)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(hostname, T_STRING);
    Check_Type(username, T_STRING);
    Check_Type(password, T_STRING);

    couchbase::origin options;
    options.hostname.assign(RSTRING_PTR(hostname), static_cast<size_t>(RSTRING_LEN(hostname)));
    options.username.assign(RSTRING_PTR(username), static_cast<size_t>(RSTRING_LEN(username)));
    options.password.assign(RSTRING_PTR(password), static_cast<size_t>(RSTRING_LEN(password)));
    auto barrier = std::make_shared<std::promise<std::error_code>>();
    auto f = barrier->get_future();
    backend->cluster->open(options, [barrier](std::error_code ec) mutable { barrier->set_value(ec); });
    if (auto ec = f.get()) {
        cb_raise_error_code(ec, "unable open cluster");
    }

    return Qnil;
}

static VALUE
cb_Backend_close(VALUE self)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);
    cb__backend_close(backend);
    return Qnil;
}

static VALUE
cb_Backend_open_bucket(VALUE self, VALUE bucket)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(bucket, T_STRING);
    std::string name(RSTRING_PTR(bucket), static_cast<size_t>(RSTRING_LEN(bucket)));

    auto barrier = std::make_shared<std::promise<std::error_code>>();
    auto f = barrier->get_future();
    backend->cluster->open_bucket(name, [barrier](std::error_code ec) mutable { barrier->set_value(ec); });
    if (auto ec = f.get()) {
        cb_raise_error_code(ec, "unable open cluster");
    }

    return Qtrue;
}

static VALUE
cb_Backend_get(VALUE self, VALUE bucket, VALUE collection, VALUE id)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(bucket, T_STRING);
    Check_Type(collection, T_STRING);
    Check_Type(id, T_STRING);

    couchbase::operations::document_id doc_id;
    doc_id.bucket.assign(RSTRING_PTR(bucket), static_cast<size_t>(RSTRING_LEN(bucket)));
    doc_id.collection.assign(RSTRING_PTR(collection), static_cast<size_t>(RSTRING_LEN(collection)));
    doc_id.key.assign(RSTRING_PTR(id), static_cast<size_t>(RSTRING_LEN(id)));

    auto barrier = std::make_shared<std::promise<couchbase::operations::get_response>>();
    auto f = barrier->get_future();
    backend->cluster->execute(couchbase::operations::get_request{ doc_id },
                              [barrier](couchbase::operations::get_response resp) mutable { barrier->set_value(resp); });
    auto resp = f.get();
    if (resp.ec) {
        cb_raise_error_code(resp.ec, fmt::format("unable fetch {}", doc_id));
    }

    VALUE res = rb_hash_new();
    rb_hash_aset(res, rb_id2sym(rb_intern("content")), rb_str_new(resp.value.data(), static_cast<long>(resp.value.size())));
    rb_hash_aset(res, rb_id2sym(rb_intern("cas")), ULONG2NUM(resp.cas));
    return res;
}

static VALUE
cb_Backend_upsert(VALUE self, VALUE bucket, VALUE collection, VALUE id, VALUE content)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(bucket, T_STRING);
    Check_Type(collection, T_STRING);
    Check_Type(id, T_STRING);

    couchbase::operations::document_id doc_id;
    doc_id.bucket.assign(RSTRING_PTR(bucket), static_cast<size_t>(RSTRING_LEN(bucket)));
    doc_id.collection.assign(RSTRING_PTR(collection), static_cast<size_t>(RSTRING_LEN(collection)));
    doc_id.key.assign(RSTRING_PTR(id), static_cast<size_t>(RSTRING_LEN(id)));
    std::string value(RSTRING_PTR(content), static_cast<size_t>(RSTRING_LEN(content)));

    auto barrier = std::make_shared<std::promise<couchbase::operations::upsert_response>>();
    auto f = barrier->get_future();
    backend->cluster->execute(couchbase::operations::upsert_request{ doc_id, value },
                              [barrier](couchbase::operations::upsert_response resp) mutable { barrier->set_value(resp); });
    auto resp = f.get();
    if (resp.ec) {
        cb_raise_error_code(resp.ec, fmt::format("unable upsert {}", doc_id));
    }

    VALUE res = rb_hash_new();
    rb_hash_aset(res, rb_id2sym(rb_intern("cas")), ULONG2NUM(resp.cas));
    return res;
}

static VALUE
cb_Backend_remove(VALUE self, VALUE bucket, VALUE collection, VALUE id)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(bucket, T_STRING);
    Check_Type(collection, T_STRING);
    Check_Type(id, T_STRING);

    couchbase::operations::document_id doc_id;
    doc_id.bucket.assign(RSTRING_PTR(bucket), static_cast<size_t>(RSTRING_LEN(bucket)));
    doc_id.collection.assign(RSTRING_PTR(collection), static_cast<size_t>(RSTRING_LEN(collection)));
    doc_id.key.assign(RSTRING_PTR(id), static_cast<size_t>(RSTRING_LEN(id)));

    auto barrier = std::make_shared<std::promise<couchbase::operations::remove_response>>();
    auto f = barrier->get_future();
    backend->cluster->execute(couchbase::operations::remove_request{ doc_id },
                              [barrier](couchbase::operations::remove_response resp) mutable { barrier->set_value(resp); });
    auto resp = f.get();
    if (resp.ec) {
        cb_raise_error_code(resp.ec, fmt::format("unable to remove {}", doc_id));
    }

    VALUE res = rb_hash_new();
    rb_hash_aset(res, rb_id2sym(rb_intern("cas")), ULONG2NUM(resp.cas));
    return res;
}

static VALUE
cb__map_subdoc_opcode(couchbase::protocol::subdoc_opcode opcode)
{
    switch (opcode) {
        case couchbase::protocol::subdoc_opcode::get:
            return rb_id2sym(rb_intern("get"));

        case couchbase::protocol::subdoc_opcode::exists:
            return rb_id2sym(rb_intern("exists"));

        case couchbase::protocol::subdoc_opcode::dict_add:
            return rb_id2sym(rb_intern("dict_add"));

        case couchbase::protocol::subdoc_opcode::dict_upsert:
            return rb_id2sym(rb_intern("dict_upsert"));

        case couchbase::protocol::subdoc_opcode::remove:
            return rb_id2sym(rb_intern("remove"));

        case couchbase::protocol::subdoc_opcode::replace:
            return rb_id2sym(rb_intern("replace"));

        case couchbase::protocol::subdoc_opcode::array_push_last:
            return rb_id2sym(rb_intern("array_push_last"));

        case couchbase::protocol::subdoc_opcode::array_push_first:
            return rb_id2sym(rb_intern("array_push_first"));

        case couchbase::protocol::subdoc_opcode::array_insert:
            return rb_id2sym(rb_intern("array_insert"));

        case couchbase::protocol::subdoc_opcode::array_add_unique:
            return rb_id2sym(rb_intern("array_add_unique"));

        case couchbase::protocol::subdoc_opcode::counter:
            return rb_id2sym(rb_intern("counter"));

        case couchbase::protocol::subdoc_opcode::get_count:
            return rb_id2sym(rb_intern("count"));
    }
    return rb_id2sym(rb_intern("unknown"));
}

static VALUE
cb__map_subdoc_status(couchbase::protocol::status status)
{
    switch (status) {
        case couchbase::protocol::status::success:
            return rb_id2sym(rb_intern("success"));

        case couchbase::protocol::status::subdoc_path_mismatch:
            return rb_id2sym(rb_intern("path_mismatch"));

        case couchbase::protocol::status::subdoc_path_invalid:
            return rb_id2sym(rb_intern("path_invalid"));

        case couchbase::protocol::status::subdoc_path_too_big:
            return rb_id2sym(rb_intern("path_too_big"));

        case couchbase::protocol::status::subdoc_value_cannot_insert:
            return rb_id2sym(rb_intern("value_cannot_insert"));

        case couchbase::protocol::status::subdoc_doc_not_json:
            return rb_id2sym(rb_intern("doc_not_json"));

        case couchbase::protocol::status::subdoc_num_range_error:
            return rb_id2sym(rb_intern("num_range"));

        case couchbase::protocol::status::subdoc_delta_invalid:
            return rb_id2sym(rb_intern("delta_invalid"));

        case couchbase::protocol::status::subdoc_path_exists:
            return rb_id2sym(rb_intern("path_exists"));

        case couchbase::protocol::status::subdoc_value_too_deep:
            return rb_id2sym(rb_intern("value_too_deep"));

        case couchbase::protocol::status::subdoc_invalid_combo:
            return rb_id2sym(rb_intern("invalid_combo"));

        case couchbase::protocol::status::subdoc_xattr_invalid_flag_combo:
            return rb_id2sym(rb_intern("xattr_invalid_flag_combo"));

        case couchbase::protocol::status::subdoc_xattr_invalid_key_combo:
            return rb_id2sym(rb_intern("xattr_invalid_key_combo"));

        case couchbase::protocol::status::subdoc_xattr_unknown_macro:
            return rb_id2sym(rb_intern("xattr_unknown_macro"));

        case couchbase::protocol::status::subdoc_xattr_unknown_vattr:
            return rb_id2sym(rb_intern("xattr_unknown_vattr"));

        case couchbase::protocol::status::subdoc_xattr_cannot_modify_vattr:
            return rb_id2sym(rb_intern("xattr_cannot_modify_vattr"));

        default:
            return rb_id2sym(rb_intern("unknown"));
    }
}

static VALUE
cb_Backend_lookup_in(VALUE self, VALUE bucket, VALUE collection, VALUE id, VALUE access_deleted, VALUE specs)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(bucket, T_STRING);
    Check_Type(collection, T_STRING);
    Check_Type(id, T_STRING);
    Check_Type(specs, T_ARRAY);
    if (RARRAY_LEN(specs) <= 0) {
        rb_raise(rb_eArgError, "Array with specs cannot be empty");
    }

    couchbase::operations::document_id doc_id;
    doc_id.bucket.assign(RSTRING_PTR(bucket), static_cast<size_t>(RSTRING_LEN(bucket)));
    doc_id.collection.assign(RSTRING_PTR(collection), static_cast<size_t>(RSTRING_LEN(collection)));
    doc_id.key.assign(RSTRING_PTR(id), static_cast<size_t>(RSTRING_LEN(id)));

    couchbase::operations::lookup_in_request req{ doc_id };
    req.access_deleted = RTEST(access_deleted);
    auto entries_size = static_cast<size_t>(RARRAY_LEN(specs));
    req.specs.entries.reserve(entries_size);
    for (size_t i = 0; i < entries_size; ++i) {
        VALUE entry = rb_ary_entry(specs, static_cast<long>(i));
        Check_Type(entry, T_HASH);
        VALUE operation = rb_hash_aref(entry, rb_id2sym(rb_intern("opcode")));
        Check_Type(operation, T_SYMBOL);
        ID operation_id = rb_sym2id(operation);
        couchbase::protocol::subdoc_opcode opcode;
        if (operation_id == rb_intern("get") || operation_id == rb_intern("get_doc")) {
            opcode = couchbase::protocol::subdoc_opcode::get;
        } else if (operation_id == rb_intern("exists")) {
            opcode = couchbase::protocol::subdoc_opcode::exists;
        } else if (operation_id == rb_intern("count")) {
            opcode = couchbase::protocol::subdoc_opcode::get_count;
        } else {
            rb_raise(rb_eArgError, "Unsupported operation for subdocument lookup");
        }
        bool xattr = RTEST(rb_hash_aref(entry, rb_id2sym(rb_intern("xattr"))));
        VALUE path = rb_hash_aref(entry, rb_id2sym(rb_intern("path")));
        Check_Type(path, T_STRING);
        req.specs.add_spec(opcode, xattr, std::string(RSTRING_PTR(path), static_cast<size_t>(RSTRING_LEN(path))));
    }

    auto barrier = std::make_shared<std::promise<couchbase::operations::lookup_in_response>>();
    auto f = barrier->get_future();
    backend->cluster->execute(req, [barrier](couchbase::operations::lookup_in_response resp) mutable { barrier->set_value(resp); });
    auto resp = f.get();
    if (resp.ec) {
        cb_raise_error_code(resp.ec, fmt::format("unable fetch {}", doc_id));
    }

    VALUE res = rb_hash_new();
    rb_hash_aset(res, rb_id2sym(rb_intern("cas")), ULONG2NUM(resp.cas));
    VALUE fields = rb_ary_new_capa(static_cast<long>(resp.fields.size()));
    rb_hash_aset(res, rb_id2sym(rb_intern("fields")), fields);
    for (size_t i = 0; i < resp.fields.size(); ++i) {
        VALUE entry = rb_hash_new();
        rb_hash_aset(entry, rb_id2sym(rb_intern("exists")), resp.fields[i].exists ? Qtrue : Qfalse);
        rb_hash_aset(
          entry, rb_id2sym(rb_intern("path")), rb_str_new(resp.fields[i].path.data(), static_cast<long>(resp.fields[i].path.size())));
        rb_hash_aset(
          entry, rb_id2sym(rb_intern("value")), rb_str_new(resp.fields[i].value.data(), static_cast<long>(resp.fields[i].value.size())));
        rb_hash_aset(entry, rb_id2sym(rb_intern("status")), cb__map_subdoc_status(resp.fields[i].status));
        if (resp.fields[i].opcode == couchbase::protocol::subdoc_opcode::get && resp.fields[i].path.empty()) {
            rb_hash_aset(entry, rb_id2sym(rb_intern("type")), rb_id2sym(rb_intern("get_doc")));
        } else {
            rb_hash_aset(entry, rb_id2sym(rb_intern("type")), cb__map_subdoc_opcode(resp.fields[i].opcode));
        }
        rb_ary_store(fields, static_cast<long>(i), entry);
    }
    return res;
}

static VALUE
cb_Backend_mutate_in(VALUE self, VALUE bucket, VALUE collection, VALUE id, VALUE access_deleted, VALUE specs)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(bucket, T_STRING);
    Check_Type(collection, T_STRING);
    Check_Type(id, T_STRING);
    Check_Type(specs, T_ARRAY);
    if (RARRAY_LEN(specs) <= 0) {
        rb_raise(rb_eArgError, "Array with specs cannot be empty");
    }

    couchbase::operations::document_id doc_id;
    doc_id.bucket.assign(RSTRING_PTR(bucket), static_cast<size_t>(RSTRING_LEN(bucket)));
    doc_id.collection.assign(RSTRING_PTR(collection), static_cast<size_t>(RSTRING_LEN(collection)));
    doc_id.key.assign(RSTRING_PTR(id), static_cast<size_t>(RSTRING_LEN(id)));

    couchbase::operations::mutate_in_request req{ doc_id };
    req.access_deleted = RTEST(access_deleted);
    auto entries_size = static_cast<size_t>(RARRAY_LEN(specs));
    req.specs.entries.reserve(entries_size);
    for (size_t i = 0; i < entries_size; ++i) {
        VALUE entry = rb_ary_entry(specs, static_cast<long>(i));
        Check_Type(entry, T_HASH);
        VALUE operation = rb_hash_aref(entry, rb_id2sym(rb_intern("opcode")));
        Check_Type(operation, T_SYMBOL);
        ID operation_id = rb_sym2id(operation);
        couchbase::protocol::subdoc_opcode opcode;
        if (operation_id == rb_intern("dict_add")) {
            opcode = couchbase::protocol::subdoc_opcode::dict_add;
        } else if (operation_id == rb_intern("dict_upsert")) {
            opcode = couchbase::protocol::subdoc_opcode::dict_upsert;
        } else if (operation_id == rb_intern("remove")) {
            opcode = couchbase::protocol::subdoc_opcode::remove;
        } else if (operation_id == rb_intern("replace")) {
            opcode = couchbase::protocol::subdoc_opcode::replace;
        } else if (operation_id == rb_intern("array_push_last")) {
            opcode = couchbase::protocol::subdoc_opcode::array_push_last;
        } else if (operation_id == rb_intern("array_push_first")) {
            opcode = couchbase::protocol::subdoc_opcode::array_push_first;
        } else if (operation_id == rb_intern("array_insert")) {
            opcode = couchbase::protocol::subdoc_opcode::array_insert;
        } else if (operation_id == rb_intern("array_add_unique")) {
            opcode = couchbase::protocol::subdoc_opcode::array_add_unique;
        } else if (operation_id == rb_intern("counter")) {
            opcode = couchbase::protocol::subdoc_opcode::counter;
        } else {
            rb_raise(rb_eArgError, "Unsupported operation for subdocument mutation: %+" PRIsVALUE, operation);
        }
        bool xattr = RTEST(rb_hash_aref(entry, rb_id2sym(rb_intern("xattr"))));
        bool create_parents = RTEST(rb_hash_aref(entry, rb_id2sym(rb_intern("create_parents"))));
        bool expand_macros = RTEST(rb_hash_aref(entry, rb_id2sym(rb_intern("expand_macros"))));
        VALUE path = rb_hash_aref(entry, rb_id2sym(rb_intern("path")));
        Check_Type(path, T_STRING);
        VALUE param = rb_hash_aref(entry, rb_id2sym(rb_intern("param")));
        if (NIL_P(param)) {
            req.specs.add_spec(opcode, xattr, std::string(RSTRING_PTR(path), static_cast<size_t>(RSTRING_LEN(path))));
        } else if (opcode == couchbase::protocol::subdoc_opcode::counter) {
            Check_Type(param, T_FIXNUM);
            req.specs.add_spec(opcode,
                               xattr,
                               create_parents,
                               expand_macros,
                               std::string(RSTRING_PTR(path), static_cast<size_t>(RSTRING_LEN(path))),
                               FIX2LONG(param));
        } else {
            Check_Type(param, T_STRING);
            req.specs.add_spec(opcode,
                               xattr,
                               create_parents,
                               expand_macros,
                               std::string(RSTRING_PTR(path), static_cast<size_t>(RSTRING_LEN(path))),
                               std::string(RSTRING_PTR(param), static_cast<size_t>(RSTRING_LEN(param))));
        }
    }

    auto barrier = std::make_shared<std::promise<couchbase::operations::mutate_in_response>>();
    auto f = barrier->get_future();
    backend->cluster->execute(req, [barrier](couchbase::operations::mutate_in_response resp) mutable { barrier->set_value(resp); });
    auto resp = f.get();
    if (resp.ec) {
        cb_raise_error_code(resp.ec, fmt::format("unable to mutate {}", doc_id));
    }

    VALUE res = rb_hash_new();
    rb_hash_aset(res, rb_id2sym(rb_intern("cas")), ULONG2NUM(resp.cas));
    if (resp.first_error_index) {
        rb_hash_aset(res, rb_id2sym(rb_intern("first_error_index")), ULONG2NUM(resp.first_error_index.value()));
    }
    VALUE fields = rb_ary_new_capa(static_cast<long>(resp.fields.size()));
    rb_hash_aset(res, rb_id2sym(rb_intern("fields")), fields);
    for (size_t i = 0; i < resp.fields.size(); ++i) {
        VALUE entry = rb_hash_new();
        rb_hash_aset(
          entry, rb_id2sym(rb_intern("path")), rb_str_new(resp.fields[i].path.data(), static_cast<long>(resp.fields[i].path.size())));
        if (resp.fields[i].opcode == couchbase::protocol::subdoc_opcode::counter) {
            rb_hash_aset(entry, rb_id2sym(rb_intern("value")), LONG2NUM(std::stoll(resp.fields[i].value)));
        } else {
            rb_hash_aset(entry,
                         rb_id2sym(rb_intern("value")),
                         rb_str_new(resp.fields[i].value.data(), static_cast<long>(resp.fields[i].value.size())));
        }
        rb_hash_aset(entry, rb_id2sym(rb_intern("status")), cb__map_subdoc_status(resp.fields[i].status));
        rb_hash_aset(entry, rb_id2sym(rb_intern("type")), cb__map_subdoc_opcode(resp.fields[i].opcode));
        rb_ary_store(fields, static_cast<long>(i), entry);
    }
    return res;
}

static int
cb__for_each_named_param(VALUE key, VALUE value, VALUE arg)
{
    auto* preq = reinterpret_cast<couchbase::operations::query_request*>(arg);
    Check_Type(key, T_STRING);
    Check_Type(value, T_STRING);
    preq->named_parameters.emplace(
      std::string_view(RSTRING_PTR(key), static_cast<std::size_t>(RSTRING_LEN(key))),
      tao::json::from_string(std::string_view(RSTRING_PTR(value), static_cast<std::size_t>(RSTRING_LEN(value)))));
    return ST_CONTINUE;
}

static VALUE
cb_Backend_query(VALUE self, VALUE statement, VALUE options)
{
    cb_backend_data* backend = nullptr;
    TypedData_Get_Struct(self, cb_backend_data, &cb_backend_type, backend);

    if (!backend->cluster) {
        rb_raise(rb_eArgError, "Cluster has been closed already");
    }

    Check_Type(statement, T_STRING);
    Check_Type(options, T_HASH);

    couchbase::operations::query_request req;
    req.statement.assign(RSTRING_PTR(statement), static_cast<size_t>(RSTRING_LEN(statement)));
    VALUE adhoc = rb_hash_aref(options, rb_id2sym(rb_intern("adhoc")));
    if (!NIL_P(adhoc)) {
        req.adhoc = RTEST(adhoc);
    }
    VALUE metrics = rb_hash_aref(options, rb_id2sym(rb_intern("metrics")));
    if (!NIL_P(metrics)) {
        req.metrics = RTEST(metrics);
    }
    VALUE readonly = rb_hash_aref(options, rb_id2sym(rb_intern("readonly")));
    if (!NIL_P(readonly)) {
        req.readonly = RTEST(readonly);
    }
    VALUE profile = rb_hash_aref(options, rb_id2sym(rb_intern("profile")));
    if (!NIL_P(profile)) {
        Check_Type(profile, T_SYMBOL);
        ID mode = rb_sym2id(profile);
        if (mode == rb_intern("phases")) {
            req.profile = couchbase::operations::query_request::profile_mode::phases;
        } else if (mode == rb_intern("timings")) {
            req.profile = couchbase::operations::query_request::profile_mode::timings;
        } else if (mode == rb_intern("off")) {
            req.profile = couchbase::operations::query_request::profile_mode::off;
        }
    }
    VALUE positional_params = rb_hash_aref(options, rb_id2sym(rb_intern("positional_parameters")));
    if (!NIL_P(positional_params)) {
        Check_Type(positional_params, T_ARRAY);
        auto entries_num = static_cast<size_t>(RARRAY_LEN(positional_params));
        req.positional_parameters.reserve(entries_num);
        for (size_t i = 0; i < entries_num; ++i) {
            VALUE entry = rb_ary_entry(positional_params, static_cast<long>(i));
            Check_Type(entry, T_STRING);
            req.positional_parameters.emplace_back(
              tao::json::from_string(std::string_view(RSTRING_PTR(entry), static_cast<std::size_t>(RSTRING_LEN(entry)))));
        }
    }
    VALUE named_params = rb_hash_aref(options, rb_id2sym(rb_intern("named_parameters")));
    if (!NIL_P(named_params)) {
        Check_Type(named_params, T_HASH);
        rb_hash_foreach(named_params, INT_FUNC(cb__for_each_named_param), reinterpret_cast<VALUE>(&req));
    }

    auto barrier = std::make_shared<std::promise<couchbase::operations::query_response>>();
    auto f = barrier->get_future();
    backend->cluster->execute(req, [barrier](couchbase::operations::query_response resp) mutable { barrier->set_value(resp); });
    auto resp = f.get();
    if (resp.ec) {
        cb_raise_error_code(resp.ec, fmt::format("unable to query: {}", req.statement.substr(0, 50)));
    }
    VALUE res = rb_hash_new();
    VALUE rows = rb_ary_new_capa(static_cast<long>(resp.payload.rows.size()));
    rb_hash_aset(res, rb_id2sym(rb_intern("rows")), rows);
    for (auto& row : resp.payload.rows) {
        rb_ary_push(rows, rb_str_new(row.data(), static_cast<long>(row.size())));
    }
    VALUE meta = rb_hash_new();
    rb_hash_aset(res, rb_id2sym(rb_intern("meta")), meta);
    rb_hash_aset(meta,
                 rb_id2sym(rb_intern("status")),
                 rb_id2sym(rb_intern2(resp.payload.meta_data.status.data(), static_cast<long>(resp.payload.meta_data.status.size()))));
    rb_hash_aset(meta,
                 rb_id2sym(rb_intern("request_id")),
                 rb_str_new(resp.payload.meta_data.request_id.data(), static_cast<long>(resp.payload.meta_data.request_id.size())));
    rb_hash_aset(
      meta,
      rb_id2sym(rb_intern("client_context_id")),
      rb_str_new(resp.payload.meta_data.client_context_id.data(), static_cast<long>(resp.payload.meta_data.client_context_id.size())));
    if (resp.payload.meta_data.signature) {
        rb_hash_aset(meta,
                     rb_id2sym(rb_intern("signature")),
                     rb_str_new(resp.payload.meta_data.signature->data(), static_cast<long>(resp.payload.meta_data.signature->size())));
    }
    if (resp.payload.meta_data.profile) {
        rb_hash_aset(meta,
                     rb_id2sym(rb_intern("profile")),
                     rb_str_new(resp.payload.meta_data.profile->data(), static_cast<long>(resp.payload.meta_data.profile->size())));
    }
    metrics = rb_hash_new();
    rb_hash_aset(meta, rb_id2sym(rb_intern("metrics")), metrics);
    rb_hash_aset(metrics,
                 rb_id2sym(rb_intern("elapsed_time")),
                 rb_str_new(resp.payload.meta_data.metrics.elapsed_time.data(),
                            static_cast<long>(resp.payload.meta_data.metrics.elapsed_time.size())));
    rb_hash_aset(metrics,
                 rb_id2sym(rb_intern("execution_time")),
                 rb_str_new(resp.payload.meta_data.metrics.execution_time.data(),
                            static_cast<long>(resp.payload.meta_data.metrics.execution_time.size())));
    rb_hash_aset(metrics, rb_id2sym(rb_intern("result_count")), ULL2NUM(resp.payload.meta_data.metrics.result_count));
    rb_hash_aset(metrics, rb_id2sym(rb_intern("result_size")), ULL2NUM(resp.payload.meta_data.metrics.result_count));
    if (resp.payload.meta_data.metrics.sort_count) {
        rb_hash_aset(metrics, rb_id2sym(rb_intern("sort_count")), ULL2NUM(*resp.payload.meta_data.metrics.sort_count));
    }
    if (resp.payload.meta_data.metrics.mutation_count) {
        rb_hash_aset(metrics, rb_id2sym(rb_intern("mutation_count")), ULL2NUM(*resp.payload.meta_data.metrics.mutation_count));
    }
    if (resp.payload.meta_data.metrics.error_count) {
        rb_hash_aset(metrics, rb_id2sym(rb_intern("error_count")), ULL2NUM(*resp.payload.meta_data.metrics.error_count));
    }
    if (resp.payload.meta_data.metrics.warning_count) {
        rb_hash_aset(metrics, rb_id2sym(rb_intern("warning_count")), ULL2NUM(*resp.payload.meta_data.metrics.warning_count));
    }

    return res;
}

static void
init_backend(VALUE mCouchbase)
{
    VALUE cBackend = rb_define_class_under(mCouchbase, "Backend", rb_cBasicObject);
    rb_define_alloc_func(cBackend, cb_Backend_allocate);
    rb_define_method(cBackend, "open", VALUE_FUNC(cb_Backend_open), 3);
    rb_define_method(cBackend, "close", VALUE_FUNC(cb_Backend_close), 0);
    rb_define_method(cBackend, "open_bucket", VALUE_FUNC(cb_Backend_open_bucket), 1);
    rb_define_method(cBackend, "get", VALUE_FUNC(cb_Backend_get), 3);
    rb_define_method(cBackend, "upsert", VALUE_FUNC(cb_Backend_upsert), 4);
    rb_define_method(cBackend, "remove", VALUE_FUNC(cb_Backend_remove), 3);
    rb_define_method(cBackend, "lookup_in", VALUE_FUNC(cb_Backend_lookup_in), 5);
    rb_define_method(cBackend, "mutate_in", VALUE_FUNC(cb_Backend_mutate_in), 5);
    rb_define_method(cBackend, "query", VALUE_FUNC(cb_Backend_query), 2);
}

extern "C" {
void
Init_libcouchbase(void)
{
    auto env_val = spdlog::details::os::getenv("SPDLOG_LEVEL");
    if (env_val.empty()) {
        spdlog::set_level(spdlog::level::critical);
    } else {
        spdlog::cfg::load_env_levels();
    }
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%P,%t] [%^%l%$] %v");

    VALUE mCouchbase = rb_define_module("Couchbase");
    init_versions(mCouchbase);
    init_backend(mCouchbase);
    init_exceptions(mCouchbase);
}
}