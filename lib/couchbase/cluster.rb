#    Copyright 2020 Couchbase, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

require "couchbase/authenticator"
require "couchbase/bucket"

require "couchbase/management/user_manager"
require "couchbase/management/bucket_manager"
require "couchbase/management/query_index_manager"
require "couchbase/management/analytics_index_manager"
require "couchbase/management/search_index_manager"

require "couchbase/search_options"
require "couchbase/query_options"
require "couchbase/analytics_options"

module Couchbase
  class Cluster
    alias inspect to_s

    # Connect to the Couchbase cluster
    #
    # @overload connect(connection_string, options)
    #   @param [String] connection_string connection string used to locate the Couchbase Cluster
    #   @param [ClusterOptions] options custom options when creating the cluster connection
    #
    # @overload connect(connection_string, username, password, options)
    #   Shortcut for {PasswordAuthenticator}
    #   @param [String] connection_string connection string used to locate the Couchbase Cluster
    #   @param [String] username name of the user
    #   @param [String] password password of the user
    #   @param [ClusterOptions, nil] options custom options when creating the cluster connection
    #
    # @return [Cluster]
    def self.connect(connection_string, *options)
      Cluster.new(connection_string, *options)
    end

    # Returns an instance of the {Bucket}
    #
    # @param [String] name name of the bucket
    #
    # @return [Bucket]
    def bucket(name)
      Bucket.new(@backend, name)
    end

    # Performs a query against the query (N1QL) services
    #
    # @param [String] statement the N1QL query statement
    # @param [QueryOptions] options the custom options for this query
    #
    # @return [QueryResult]
    def query(statement, options = QueryOptions.new)
      resp = @backend.document_query(statement, {
        timeout: options.timeout,
        adhoc: options.adhoc,
        client_context_id: options.client_context_id,
        max_parallelism: options.max_parallelism,
        readonly: options.readonly,
        flex_index: options.flex_index,
        scan_wait: options.scan_wait,
        scan_cap: options.scan_cap,
        pipeline_batch: options.pipeline_batch,
        pipeline_cap: options.pipeline_cap,
        metrics: options.metrics,
        profile: options.profile,
        positional_parameters: options.export_positional_parameters,
        named_parameters: options.export_named_parameters,
        scope_qualifier: options.scope_qualifier,
        raw_parameters: options.raw_parameters,
        scan_consistency: options.scan_consistency,
        mutation_state: options.mutation_state&.to_a,
      })

      QueryResult.new do |res|
        res.meta_data = QueryMetaData.new do |meta|
          meta.status = resp[:meta][:status]
          meta.request_id = resp[:meta][:request_id]
          meta.client_context_id = resp[:meta][:client_context_id]
          meta.signature = JSON.parse(resp[:meta][:signature]) if resp[:meta][:signature]
          meta.profile = JSON.parse(resp[:meta][:profile]) if resp[:meta][:profile]
          meta.metrics = QueryMetrics.new do |metrics|
            if resp[:meta][:metrics]
              metrics.elapsed_time = resp[:meta][:metrics][:elapsed_time]
              metrics.execution_time = resp[:meta][:metrics][:execution_time]
              metrics.sort_count = resp[:meta][:metrics][:sort_count]
              metrics.result_count = resp[:meta][:metrics][:result_count]
              metrics.result_size = resp[:meta][:metrics][:result_size]
              metrics.mutation_count = resp[:meta][:metrics][:mutation_count]
              metrics.error_count = resp[:meta][:metrics][:error_count]
              metrics.warning_count = resp[:meta][:metrics][:warning_count]
            end
          end
          res[:warnings] = resp[:warnings].map { |warn| QueryWarning.new(warn[:code], warn[:message]) } if resp[:warnings]
        end
        res.instance_variable_set("@rows", resp[:rows])
      end
    end

    # Performs an analytics query
    #
    # @param [String] statement the N1QL query statement
    # @param [AnalyticsOptions] options the custom options for this query
    #
    # @return [AnalyticsResult]
    def analytics_query(statement, options = AnalyticsOptions.new)
      resp = @backend.document_analytics(statement, {
        timeout: options.timeout,
        client_context_id: options.client_context_id,
        scan_consistency: options.scan_consistency,
        readonly: options.readonly,
        priority: options.priority,
        positional_parameters: options.export_positional_parameters,
        named_parameters: options.export_named_parameters,
        raw_parameters: options.raw_parameters,
      })

      AnalyticsResult.new do |res|
        res.transcoder = options.transcoder
        res.meta_data = AnalyticsMetaData.new do |meta|
          meta.status = resp[:meta][:status]
          meta.request_id = resp[:meta][:request_id]
          meta.client_context_id = resp[:meta][:client_context_id]
          meta.signature = JSON.parse(resp[:meta][:signature]) if resp[:meta][:signature]
          meta.profile = JSON.parse(resp[:meta][:profile]) if resp[:meta][:profile]
          meta.metrics = AnalyticsMetrics.new do |metrics|
            if resp[:meta][:metrics]
              metrics.elapsed_time = resp[:meta][:metrics][:elapsed_time]
              metrics.execution_time = resp[:meta][:metrics][:execution_time]
              metrics.result_count = resp[:meta][:metrics][:result_count]
              metrics.result_size = resp[:meta][:metrics][:result_size]
              metrics.error_count = resp[:meta][:metrics][:error_count]
              metrics.warning_count = resp[:meta][:metrics][:warning_count]
              metrics.processed_objects = resp[:meta][:metrics][:processed_objects]
            end
          end
          res[:warnings] = resp[:warnings].map { |warn| QueryWarning.new(warn[:code], warn[:message]) } if resp[:warnings]
        end
        res.instance_variable_set("@rows", resp[:rows])
      end
    end

    # Performs a Full Text Search (FTS) query
    #
    # @param [String] index_name the name of the search index
    # @param [SearchQuery] query the query tree
    # @param [SearchOptions] options the query tree
    #
    # @return [SearchResult]
    def search_query(index_name, query, options = SearchOptions.new)
      resp = @backend.document_search(index_name, JSON.generate(query), {
        timeout: options.timeout,
        limit: options.limit,
        skip: options.skip,
        explain: options.explain,
        disable_scoring: options.disable_scoring,
        highlight_style: options.highlight_style,
        highlight_fields: options.highlight_fields,
        fields: options.fields,
        sort: options.sort&.map { |v| JSON.generate(v) },
        facets: options.facets&.map { |(k, v)| [k, JSON.generate(v)] },
        scan_consistency: options.scan_consistency,
        mutation_state: options.mutation_state&.to_a,
      })

      SearchResult.new do |res|
        res.meta_data = SearchMetaData.new do |meta|
          meta.metrics.max_score = resp[:meta_data][:metrics][:max_score]
          meta.metrics.error_partition_count = resp[:meta_data][:metrics][:error_partition_count]
          meta.metrics.success_partition_count = resp[:meta_data][:metrics][:success_partition_count]
          meta.metrics.took = resp[:meta_data][:metrics][:took]
          meta.metrics.total_rows = resp[:meta_data][:metrics][:total_rows]
          meta.errors = resp[:meta_data][:errors]
        end
        res.rows = resp[:rows].map do |r|
          SearchRow.new do |row|
            row.transcoder = options.transcoder
            row.index = r[:index]
            row.id = r[:id]
            row.score = r[:score]
            row.fragments = r[:fragments]
            row.locations = SearchRowLocations.new(
              r[:locations].map do |loc|
                SearchRowLocation.new do |location|
                  location.field = loc[:field]
                  location.term = loc[:term]
                  location.position = loc[:position]
                  location.start_offset = loc[:start_offset]
                  location.end_offset = loc[:end_offset]
                  location.array_positions = loc[:array_positions]
                end
              end
            )
            row.instance_variable_set("@fields", r[:fields])
            row.explanation = JSON.parse(r[:explanation]) if r[:explanation]
          end
        end
        if resp[:facets]
          res.facets = resp[:facets].each_with_object({}) do |(k, v), o|
            facet = case options.facets[k]
                    when SearchFacet::SearchFacetTerm
                      SearchFacetResult::TermFacetResult.new do |f|
                        f.terms =
                          if v[:terms]
                            v[:terms].map do |t|
                              SearchFacetResult::TermFacetResult::TermFacet.new(t[:term], t[:count])
                            end
                          else
                            []
                          end
                      end
                    when SearchFacet::SearchFacetDateRange
                      SearchFacetResult::DateRangeFacetResult.new do |f|
                        f.date_ranges =
                          if v[:date_ranges]
                            v[:date_ranges].map do |r|
                              SearchFacetResult::DateRangeFacetResult::DateRangeFacet.new(r[:name], r[:count], r[:start_time], r[:end_time])
                            end
                          else
                            []
                          end
                      end
                    when SearchFacet::SearchFacetNumericRange
                      SearchFacetResult::NumericRangeFacetResult.new do |f|
                        f.numeric_ranges =
                          if v[:numeric_ranges]
                            v[:numeric_ranges].map do |r|
                              SearchFacetResult::NumericRangeFacetResult::NumericRangeFacet.new(r[:name], r[:count], r[:min], r[:max])
                            end
                          else
                            []
                          end
                      end
                    else
                      next # ignore unknown facet result
                    end
            facet.name = v[:name]
            facet.field = v[:field]
            facet.total = v[:total]
            facet.missing = v[:missing]
            facet.other = v[:other]
            o[k] = facet
          end
        end
      end
    end

    # @return [Management::UserManager]
    def users
      Management::UserManager.new(@backend)
    end

    # @return [Management::BucketManager]
    def buckets
      Management::BucketManager.new(@backend)
    end

    # @return [Management::QueryIndexManager]
    def query_indexes
      Management::QueryIndexManager.new(@backend)
    end

    # @return [Management::AnalyticsIndexManager]
    def analytics_indexes
      Management::AnalyticsIndexManager.new(@backend)
    end

    # @return [Management::SearchIndexManager]
    def search_indexes
      Management::SearchIndexManager.new(@backend)
    end

    # Closes all connections to services and free allocated resources
    #
    # @return [void]
    def disconnect
      @backend.close
    end

    # Creates diagnostic report that can be used to determine the health of the network connections.
    #
    # It does not proactively perform any I/O against the network
    #
    # @return [DiagnosticsResult]
    def diagnostics(options = DiagnosticsOptions.new)
      resp = @backend.diagnostics(options.report_id)
      pp resp
      DiagnosticsResult.new do |res|
        res.version = resp[:version]
        res.id = resp[:id]
        res.sdk = resp[:sdk]
        resp[:services].each do |type, svcs|
          res.services[type] = svcs.map do |svc|
            DiagnosticsResult::ServiceInfo.new do |info|
              info.id = svc[:id]
              info.state = svc[:state]
              info.last_activity_us = svc[:last_activity_us]
              info.remote = svc[:remote]
              info.local = svc[:local]
            end
          end
        end
      end
    end

    class ClusterOptions
      attr_accessor :authenticator

      # @yieldparam [ClusterOptions] self
      def initialize
        yield self if block_given?
      end

      def authenticate(username, password)
        @authenticator = PasswordAuthenticator.new(username, password)
        self
      end
    end

    class DiagnosticsOptions
      # @return [String] Holds custom report id.
      attr_accessor :report_id

      # @yieldparam [DiagnosticsOptions] self
      def initialize
        yield self if block_given?
      end
    end

    class DiagnosticsResult
      class ServiceInfo
        # @return [String] endpoint unique identifier
        attr_accessor :id

        # Possible states are:
        #
        # :disconnected:: the endpoint is not reachable
        # :connecting:: currently connecting (includes auth, handshake, etc.)
        # :connected:: connected and ready
        # :disconnecting:: disconnecting (after being connected)
        #
        # @return [Symbol] state of the endpoint
        attr_accessor :state

        # @return [Integer] how long ago the endpoint was active (in microseconds)
        attr_accessor :last_activity_us

        # @return [String] remote address of the connection
        attr_accessor :remote

        # @return [String] local address of the connection
        attr_accessor :local

        # @yieldparam [ServiceInfo] self
        def initialize
          yield self if block_given?
        end

        def to_json(*args)
          data = {
            id: @id,
            state: @state,
            remote: @remote,
            local: @local,
          }
          data[:last_activity_us] = @last_activity_us if defined? @last_activity_us
          data.to_json(*args)
        end
      end

      # @return [String] report id
      attr_accessor :id

      # @return [String] SDK identifier
      attr_accessor :sdk

      # Returns information about currently service endpoints, that known to the library at the moment.
      #
      # :kv:: Key/Value data service
      # :query:: N1QL query service
      # :analytics:: Analtyics service
      # :search:: Full text search service
      # :views:: Views service
      # :mgmt:: Management service
      #
      # @return [Hash<Symbol, ServiceInfo>] map service types to info
      attr_accessor :services

      # @yieldparam [DiagnosticsResult] self
      def initialize
        @services = Hash.new
        yield self if block_given?
      end

      # @api private
      # @return [Integer] version
      attr_accessor :version

      def to_json(*args)
        {
          version: @version,
          id: @id,
          sdk: @sdk,
          services: @services,
        }.to_json(*args)
      end
    end

    private

    # Initialize {Cluster} object
    #
    # @overload new(connection_string, options)
    #   @param [String] connection_string connection string used to locate the Couchbase Cluster
    #   @param [ClusterOptions] options custom options when creating the cluster connection
    #
    # @overload new(connection_string, username, password, options)
    #   Shortcut for {PasswordAuthenticator}
    #   @param [String] connection_string connection string used to locate the Couchbase Cluster
    #   @param [String] username name of the user
    #   @param [String] password password of the user
    #   @param [ClusterOptions, nil] options custom options when creating the cluster connection
    def initialize(connection_string, *args)
      credentials = {}

      options = args.shift
      case options
      when String
        credentials[:username] = options
        credentials[:password] = args.shift
        raise ArgumentError, "missing username" unless credentials[:username]
        raise ArgumentError, "missing password" unless credentials[:password]
      when ClusterOptions
        authenticator = options&.authenticator
        case authenticator
        when PasswordAuthenticator
          credentials[:username] = authenticator&.username
          raise ArgumentError, "missing username" unless credentials[:username]

          credentials[:password] = authenticator&.password
          raise ArgumentError, "missing password" unless credentials[:password]

        when CertificateAuthenticator
          credentials[:certificate_path] = authenticator&.certificate_path
          raise ArgumentError, "missing certificate path" unless credentials[:certificate_path]

          credentials[:key_path] = authenticator&.key_path
          raise ArgumentError, "missing key path" unless credentials[:key_path]

        else
          raise ArgumentError, "options must have authenticator configured"
        end
      else
        raise ArgumentError, "unexpected second argument, have to be String or ClusterOptions"
      end

      @backend = Backend.new
      @backend.open(connection_string, credentials, {})
    end
  end
end
