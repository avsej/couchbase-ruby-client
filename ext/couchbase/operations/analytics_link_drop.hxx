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

#include <utils/url_codec.hxx>

namespace couchbase::operations
{
struct analytics_link_drop_response {
    struct problem {
        std::uint32_t code;
        std::string message;
    };

    error_context::http ctx;
    std::string status{};
    std::vector<problem> errors{};
};

struct analytics_link_drop_request {
    using response_type = analytics_link_drop_response;
    using encoded_request_type = io::http_request;
    using encoded_response_type = io::http_response;
    using error_context_type = error_context::http;

    static const inline service_type type = service_type::analytics;

    std::string client_context_id{ uuid::to_string(uuid::random()) };
    std::chrono::milliseconds timeout{ timeout_defaults::management_timeout };

    std::string link_name{};
    std::string dataverse_name{};

    [[nodiscard]] std::error_code encode_to(encoded_request_type& encoded, http_context& /* context */) const
    {
        encoded.headers["content-type"] = "application/x-www-form-urlencoded";
        encoded.headers["accept"] = "application/json";
        encoded.method = "DELETE";
        if (std::count(dataverse_name.begin(), dataverse_name.end(), '/') == 0) {
            std::map<std::string, std::string> values{
                { "dataverse", dataverse_name },
                { "name", link_name },
            };
            encoded.body = utils::string_codec::v2::form_encode(values);
            encoded.path = "/analytics/link";
        } else {
            encoded.path = fmt::format("/analytics/link/{}/{}", utils::string_codec::v2::path_escape(dataverse_name), link_name);
        }
        return {};
    }
};

analytics_link_drop_response
make_response(error_context::http&& ctx,
              const analytics_link_drop_request& /* request */,
              typename analytics_link_drop_request::encoded_response_type&& encoded)
{
    analytics_link_drop_response response{ std::move(ctx) };
    if (!response.ctx.ec) {
        if (encoded.body.empty() && response.ctx.http_status == 200) {
            return response;
        }
        tao::json::value payload{};
        try {
            payload = tao::json::from_string(encoded.body);
        } catch (const tao::json::pegtl::parse_error&) {
            auto colon = encoded.body.find(':');
            if (colon == std::string::npos) {
                response.ctx.ec = error::common_errc::parsing_failure;
                return response;
            }
            auto code = static_cast<std::uint32_t>(std::stoul(encoded.body, 0));
            auto msg = encoded.body.substr(colon + 1);
            response.errors.emplace_back(analytics_link_drop_response::problem{ code, msg });
        }
        if (payload) {
            response.status = payload.at("status").get_string();
            if (response.status != "success") {
                if (auto* errors = payload.find("errors"); errors != nullptr && errors->is_array()) {
                    for (const auto& error : errors->get_array()) {
                        analytics_link_drop_response::problem err{
                            error.at("code").as<std::uint32_t>(),
                            error.at("msg").get_string(),
                        };
                        response.errors.emplace_back(err);
                    }
                }
            }
        }
        bool link_not_found = false;
        bool dataverse_does_not_exist = false;
        for (const auto& err : response.errors) {
            switch (err.code) {
                case 24006: /* Link [string] does not exist */
                    link_not_found = true;
                    break;
                case 24034: /* Cannot find dataverse with name [string] */
                    dataverse_does_not_exist = true;
                    break;
            }
        }
        if (dataverse_does_not_exist) {
            response.ctx.ec = error::analytics_errc::dataverse_not_found;
        } else if (link_not_found) {
            response.ctx.ec = error::analytics_errc::link_not_found;
        } else {
            response.ctx.ec = error::common_errc::internal_server_failure;
        }
    }
    return response;
}

} // namespace couchbase::operations
