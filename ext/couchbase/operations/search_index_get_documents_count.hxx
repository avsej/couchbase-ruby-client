/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2020-2021 Couchbase, Inc.
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

#pragma once

#include <tao/json.hpp>

#include <version.hxx>

namespace couchbase::operations
{
struct search_index_get_documents_count_response {
    error_context::http ctx;
    std::string status{};
    std::uint64_t count{ 0 };
    std::string error{};
};

struct search_index_get_documents_count_request {
    using response_type = search_index_get_documents_count_response;
    using encoded_request_type = io::http_request;
    using encoded_response_type = io::http_response;
    using error_context_type = error_context::http;

    static const inline service_type type = service_type::search;

    std::string client_context_id{ uuid::to_string(uuid::random()) };
    std::chrono::milliseconds timeout{ timeout_defaults::management_timeout };

    std::string index_name;

    [[nodiscard]] std::error_code encode_to(encoded_request_type& encoded, http_context& /* context */) const
    {
        encoded.method = "GET";
        encoded.path = fmt::format("/api/index/{}/count", index_name);
        return {};
    }
};

search_index_get_documents_count_response
make_response(error_context::http&& ctx,
              const search_index_get_documents_count_request& /* request */,
              search_index_get_documents_count_request::encoded_response_type&& encoded)
{
    search_index_get_documents_count_response response{ std::move(ctx) };
    if (!response.ctx.ec) {
        switch (encoded.status_code) {
            case 200: {
                tao::json::value payload{};
                try {
                    payload = tao::json::from_string(encoded.body);
                } catch (const tao::json::pegtl::parse_error& e) {
                    response.ctx.ec = error::common_errc::parsing_failure;
                    return response;
                }
                response.status = payload.at("status").get_string();
                if (response.status == "ok") {
                    response.count = payload.at("count").get_unsigned();
                    return response;
                }
            } break;
            case 400:
            case 500: {
                tao::json::value payload{};
                try {
                    payload = tao::json::from_string(encoded.body);
                } catch (const tao::json::pegtl::parse_error& e) {
                    response.ctx.ec = error::common_errc::parsing_failure;
                    return response;
                }
                response.status = payload.at("status").get_string();
                response.error = payload.at("error").get_string();
                if (response.error.find("index not found") != std::string::npos) {
                    response.ctx.ec = error::common_errc::index_not_found;
                    return response;
                } else if (response.error.find("no planPIndexes for indexName") != std::string::npos) {
                    response.ctx.ec = error::search_errc::index_not_ready;
                    return response;
                }
            } break;
        }
        response.ctx.ec = error::common_errc::internal_server_failure;
    }
    return response;
}

} // namespace couchbase::operations
