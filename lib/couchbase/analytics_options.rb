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

require 'couchbase/json_transcoder'

module Couchbase
  class Cluster
    class AnalyticsOptions
      # @return [Integer] Timeout in milliseconds
      attr_accessor :timeout

      # @return [String] Provides a custom client context ID for this query
      attr_accessor :client_context_id

      # @return [:not_bounded, :request_plus] specifies level of consistency for the query
      attr_accessor :scan_consistency

      # @return [Integer] The maximum duration (in milliseconds) the query engine is willing to wait before failing.
      attr_accessor :scan_wait

      # @return [Boolean] Allows explicitly marking a query as being readonly and not mutating and documents on the server side.
      attr_accessor :readonly

      # @return [Boolean] Allows to give certain requests higher priority than others
      attr_accessor :priority

      # @return [JsonTranscoder] transcoder to use on rows
      attr_accessor :transcoder

      # @yieldparam [AnalyticsOptions] self
      def initialize
        @transcoder = JsonTranscoder.new
        @raw_parameters = {}
        @positional_parameters = nil
        @named_parameters = nil
        @scan_consistency = nil
        yield self if block_given?
      end

      # Sets positional parameters for the query
      #
      # @param [Array] positional the list of parameters that have to be substituted in the statement
      def positional_parameters(positional)
        @positional_parameters = positional
        @named_parameters = nil
      end

      # Sets named parameters for the query
      #
      # @param [Hash] named the key/value map of the parameters to substitute in the statement
      def named_parameters(named)
        @named_parameters = named
        @positional_parameters = nil
      end

      # Allows providing custom JSON key/value pairs for advanced usage
      #
      # @param [String] key the parameter name (key of the JSON property)
      # @param [Object] value the parameter value (value of the JSON property)
      def raw(key, value)
        @raw_parameters[key] = JSON.generate(value)
      end
    end

    class AnalyticsWarning
      # @return [Integer]
      attr_accessor :code

      # @return [String]
      attr_accessor :message

      # @param [Integer] code
      # @param [String] message
      def initialize(code, message)
        @code = code
        @message = message
      end
    end

    class AnalyticsMetrics
      # @return [Integer] duration in milliseconds
      attr_accessor :elapsed_time

      # @return [Integer] duration in milliseconds
      attr_accessor :execution_time

      # @return [Integer]
      attr_accessor :result_count

      # @return [Integer]
      attr_accessor :result_size

      # @return [Integer]
      attr_accessor :error_count

      # @return [Integer]
      attr_accessor :processed_objects

      # @return [Integer]
      attr_accessor :warning_count

      # @yieldparam [AnalyticsMetrics] self
      def initialize
        yield self if block_given?
      end
    end

    class AnalyticsMetaData
      # @return [String]
      attr_accessor :request_id

      # @return [String]
      attr_accessor :client_context_id

      # @return [:running, :success, :errors, :completed, :stopped, :timeout, :closed, :fatal, :aborted, :unknown]
      attr_accessor :status

      # @return [Hash] returns the signature as returned by the query engine which is then decoded as JSON object
      attr_accessor :signature

      # @return [Array<AnalyticsWarning>]
      attr_accessor :warnings

      # @return [AnalyticsMetrics]
      attr_accessor :metrics

      # @yieldparam [AnalyticsMetaData] self
      def initialize
        yield self if block_given?
      end
    end

    class AnalyticsResult
      # @return [AnalyticsMetaData]
      attr_accessor :meta_data

      attr_accessor :transcoder

      # Returns all rows converted using a transcoder
      #
      # @return [Array]
      def rows(transcoder = self.transcoder)
        @rows.lazy.map { |row| transcoder.decode(row, 0) }
      end

      # @yieldparam [AnalyticsResult] self
      def initialize
        @transcoder = JsonTranscoder.new
        yield self if block_given?
      end
    end
  end
end
