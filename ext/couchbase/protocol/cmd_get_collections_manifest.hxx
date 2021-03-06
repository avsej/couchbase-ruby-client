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

#include <gsl/gsl_assert>

#include <protocol/client_opcode.hxx>
#include <protocol/cmd_info.hxx>
#include <protocol/status.hxx>

#include <collections_manifest.hxx>

namespace couchbase::protocol
{

class get_collections_manifest_response_body
{
  public:
    static const inline client_opcode opcode = client_opcode::get_collections_manifest;

  private:
    collections_manifest manifest_;

  public:
    [[nodiscard]] const couchbase::collections_manifest& manifest() const
    {
        return manifest_;
    }

    bool parse(protocol::status status,
               const header_buffer& header,
               std::uint8_t framing_extras_size,
               std::uint16_t key_size,
               std::uint8_t extras_size,
               const std::vector<uint8_t>& body,
               const cmd_info& /* info */)
    {
        Expects(header[1] == static_cast<uint8_t>(opcode));
        if (status == protocol::status::success) {
            std::vector<uint8_t>::difference_type offset = framing_extras_size + key_size + extras_size;
            manifest_ = tao::json::from_string(std::string(body.begin() + offset, body.end())).as<collections_manifest>();
            return true;
        }
        return false;
    }
};

class get_collections_manifest_request_body
{
  public:
    using response_body_type = get_collections_manifest_response_body;
    static const inline client_opcode opcode = client_opcode::get_collections_manifest;

    [[nodiscard]] const std::string& key() const
    {
        return empty_string;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& framing_extras() const
    {
        return empty_buffer;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& extras() const
    {
        return empty_buffer;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& value() const
    {
        return empty_buffer;
    }

    [[nodiscard]] std::size_t size() const
    {
        return 0;
    }
};

} // namespace couchbase::protocol
