# Generated by the protocol buffer compiler.  DO NOT EDIT!
# Source: couchbase/query/v1/query.proto for package 'Couchbase.Protostellar.Generated.Query.V1'

require 'grpc'
require 'couchbase/protostellar/generated/query/v1/query_pb'

module Couchbase
  module Protostellar
    module Generated
      module Query
        module V1
          module QueryService
            class Service

              include ::GRPC::GenericService

              self.marshal_class_method = :encode
              self.unmarshal_class_method = :decode
              self.service_name = 'couchbase.query.v1.QueryService'

              rpc :Query, ::Couchbase::Protostellar::Generated::Query::V1::QueryRequest, stream(::Couchbase::Protostellar::Generated::Query::V1::QueryResponse)
            end

            Stub = Service.rpc_stub_class
          end
        end
      end
    end
  end
end