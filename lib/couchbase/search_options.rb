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

module Couchbase
  class Cluster
    class SearchQuery
      # Prepare {MatchQuery} body
      #
      # @param [String] match
      # @yieldparam [MatchQuery] query
      #
      # @return [MatchQuery]
      def self.match(match, &block)
        MatchQuery.new(match, &block)
      end

      # A match query analyzes the input text and uses that analyzed text to query the index.
      class MatchQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @return [String]
        attr_accessor :analyzer

        # @return [Integer]
        attr_accessor :prefix_length

        # @return [Integer]
        attr_accessor :fuzziness

        # @param [String] match
        # @yieldparam [MatchQuery] self
        def initialize(match)
          super()
          @match = match
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"match" => @match}
          data["boost"] = boost if boost
          data["field"] = field if field
          data["analyzer"] = analyzer if analyzer
          if fuzziness
            data["fuzziness"] = fuzziness
            data["prefix_length"] = prefix_length if prefix_length
          end
          data.to_json(*args)
        end
      end

      # Prepare {MatchPhraseQuery} body
      #
      # @param [String] match_phrase
      # @yieldparam [MatchPhraseQuery] query
      #
      # @return [MatchPhraseQuery]
      def self.match_phrase(match_phrase, &block)
        MatchPhraseQuery.new(match_phrase, &block)
      end

      # The input text is analyzed and a phrase query is built with the terms resulting from the analysis.
      class MatchPhraseQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @return [String]
        attr_accessor :analyzer

        # @param [String] match_phrase
        #
        # @yieldparam [MatchPhraseQuery] self
        def initialize(match_phrase)
          super()
          @match_phrase = match_phrase
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"match_phrase" => @match_phrase}
          data["boost"] = boost if boost
          data["field"] = field if field
          data["analyzer"] = analyzer if analyzer
          data.to_json(*args)
        end
      end

      # Prepare {RegexpQuery} body
      #
      # @param [String] regexp
      # @yieldparam [RegexpQuery] query
      #
      # @return [RegexpQuery]
      def self.regexp(regexp, &block)
        RegexpQuery.new(regexp, &block)
      end

      # Finds documents containing terms that match the specified regular expression.
      class RegexpQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @param [String] regexp
        #
        # @yieldparam [RegexpQuery] self
        def initialize(regexp)
          super()
          @regexp = regexp
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"regexp" => @regexp}
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {QueryStringQuery} body
      #
      # @param [String] query_string
      # @yieldparam [QueryStringQuery] query
      #
      # @return [QueryStringQuery]
      def self.query_string(query_string, &block)
        QueryStringQuery.new(query_string, &block)
      end

      # The query string query allows humans to describe complex queries using a simple syntax.
      class QueryStringQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @param [String] query_string
        #
        # @yieldparam [QueryStringQuery] self
        def initialize(query_string)
          super()
          @query_string = query_string
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"query" => @query_string}
          data["boost"] = boost if boost
          data.to_json(*args)
        end
      end

      # Prepare {WildcardQuery} body
      #
      # @param [String] wildcard
      # @yieldparam [WildcardQuery] query
      #
      # @return [WildcardQuery]
      def self.wildcard(wildcard, &block)
        WildcardQuery.new(wildcard, &block)
      end

      # Interprets * and ? wildcards as found in a lot of applications, for an easy implementation of such a search feature.
      class WildcardQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @param [String] wildcard
        #
        # @yieldparam [WildcardQuery] self
        def initialize(wildcard)
          super()
          @wildcard = wildcard
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"wildcard" => @wildcard}
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {DocIdQuery} body
      #
      # @param [String...] doc_ids
      # @yieldparam [DocIdQuery] query
      #
      # @return [DocIdQuery]
      def self.doc_id(*doc_ids)
        DocIdQuery.new(*doc_ids)
      end

      # Allows to restrict matches to a set of specific documents.
      class DocIdQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @param [String...] doc_ids
        #
        # @yieldparam [DocIdQuery] self
        def initialize(*doc_ids)
          super()
          @doc_ids = doc_ids
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"doc_ids" => @doc_ids.flatten.uniq}
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {BooleanFieldQuery} body
      #
      # @param [Boolean] value
      # @yieldparam [BooleanFieldQuery] query
      #
      # @return [BooleanFieldQuery]
      def self.boolean_field(value)
        BooleanFieldQuery.new(value)
      end

      # Allow to match `true`/`false` in a field mapped as boolean.
      class BooleanFieldQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @param [Boolean] value
        #
        # @yieldparam [BooleanFieldQuery] self
        def initialize(value)
          super()
          @value = value
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"bool" => @value}
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {DateRangeQuery} body
      #
      # @yieldparam [DateRangeQuery] query
      #
      # @return [DateRangeQuery]
      def self.date_range(&block)
        DateRangeQuery.new(&block)
      end

      # The date range query finds documents containing a date value in the specified field within the specified range.
      class DateRangeQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @return [String]
        attr_accessor :date_time_parser

        # Sets the lower boundary of the range.
        #
        # @note The lower boundary is considered inclusive by default on the server side.
        #
        # @param [Time, String] time_point start time. When +Time+ object is passed {#date_time_parser} must be +nil+ (to use server
        #   default)
        # @param [Boolean] inclusive
        def start_time(time_point, inclusive = nil)
          @start_time = time_point
          @start_inclusive = inclusive
        end

        # Sets the upper boundary of the range.
        #
        # @note The upper boundary is considered exclusive by default on the server side.
        #
        # @param [Time, String] time_point end time. When +Time+ object is passed {#date_time_parser} must be +nil+ (to use server default)
        # @param [Boolean] inclusive
        def end_time(time_point, inclusive = nil)
          @end_time = time_point
          @end_inclusive = inclusive
        end

        # @yieldparam [DateRangeQuery] self
        def initialize
          super
          @start_time = nil
          @start_inclusive = nil
          @end_time = nil
          @end_inclusive = nil
          yield self if block_given?
        end

        DATE_FORMAT_RFC3339 = "%Y-%m-%dT%H:%M:%S%:z".freeze

        # @return [String]
        def to_json(*args)
          raise ArgumentError, "either start_time or end_time must be set for DateRangeQuery" if @start_time.nil? && @end_time.nil?

          data = {}
          data["boost"] = boost if boost
          data["field"] = field if field
          data["datetime_parser"] = date_time_parser if date_time_parser
          if @start_time
            data["start"] = if @start_time.respond_to?(:strftime)
                              @start_time.strftime(DATE_FORMAT_RFC3339)
                            else
                              @start_time
                            end
            data["inclusive_start"] = @start_inclusive unless @start_inclusive.nil?
          end
          if @end_time
            data["end"] = if @end_time.respond_to?(:strftime)
                            @end_time.strftime(DATE_FORMAT_RFC3339)
                          else
                            @end_time
                          end
            data["inclusive_end"] = @end_inclusive unless @end_inclusive.nil?
          end
          data.to_json(*args)
        end
      end

      # Prepare {NumericRangeQuery} body
      #
      # @yieldparam [NumericRangeQuery] query
      #
      # @return [NumericRangeQuery]
      def self.numeric_range(&block)
        NumericRangeQuery.new(&block)
      end

      # The numeric range query finds documents containing a numeric value in the specified field within the specified range.
      class NumericRangeQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # Sets lower bound of the range.
        #
        # The lower boundary is considered inclusive by default on the server side.
        #
        # @param [Numeric] lower_bound
        # @param [Boolean] inclusive
        def min(lower_bound, inclusive = nil)
          @min = lower_bound
          @min_inclusive = inclusive
        end

        # Sets upper bound of the range.
        #
        # The upper boundary is considered exclusive by default on the server side.
        #
        # @param [Numeric] upper_bound
        # @param [Boolean] inclusive
        def max(upper_bound, inclusive = nil)
          @max = upper_bound
          @max_inclusive = inclusive
        end

        # @yieldparam [NumericRangeQuery] self
        def initialize
          super
          @min = nil
          @min_inclusive = nil
          @max = nil
          @max_inclusive = nil
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          raise ArgumentError, "either min or max must be set for NumericRangeQuery" if @min.nil? && @max.nil?

          data = {}
          data["boost"] = boost if boost
          data["field"] = field if field
          if @min
            data["min"] = @min
            data["inclusive_min"] = @min_inclusive unless @min_inclusive.nil?
          end
          if @max
            data["max"] = @max
            data["inclusive_max"] = @max_inclusive unless @max_inclusive.nil?
          end
          data.to_json(*args)
        end
      end

      # Prepare {TermRangeQuery} body
      #
      # @yieldparam [TermRangeQuery] query
      #
      # @return [TermRangeQuery]
      def self.term_range(&block)
        TermRangeQuery.new(&block)
      end

      # The term range query finds documents containing a string value in the specified field within the specified range.
      class TermRangeQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # Sets lower bound of the range.
        #
        # The lower boundary is considered inclusive by default on the server side.
        #
        # @param [String] lower_bound
        # @param [Boolean] inclusive
        def min(lower_bound, inclusive = nil)
          @min = lower_bound
          @min_inclusive = inclusive
        end

        # Sets upper bound of the range.
        #
        # The upper boundary is considered exclusive by default on the server side.
        #
        # @param [String] upper_bound
        # @param [Boolean] inclusive
        def max(upper_bound, inclusive = nil)
          @max = upper_bound
          @max_inclusive = inclusive
        end

        # @yieldparam [TermRangeQuery] self
        def initialize
          super
          @min = nil
          @min_inclusive = nil
          @max = nil
          @max_inclusive = nil
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          raise ArgumentError, "either min or max must be set for TermRangeQuery" if @min.nil? && @max.nil?

          data = {}
          data["boost"] = boost if boost
          data["field"] = field if field
          if @min
            data["min"] = @min
            data["inclusive_min"] = @min_inclusive unless @min_inclusive.nil?
          end
          if @max
            data["max"] = @max
            data["inclusive_max"] = @max_inclusive unless @max_inclusive.nil?
          end
          data.to_json(*args)
        end
      end

      # Prepare {GeoDistanceQuery} body
      #
      # @yieldparam [GeoDistanceQuery] query
      #
      # @param [Float] latitude location latitude
      # @param [Float] longitude location longitude
      # @param [String] distance how big is area (number with units)
      #
      # @return [GeoDistanceQuery]
      def self.geo_distance(longitude, latitude, distance, &block)
        GeoDistanceQuery.new(longitude, latitude, distance, &block)
      end

      # Finds `geopoint` indexed matches around a point with the given distance.
      class GeoDistanceQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @yieldparam [GeoDistanceQuery] self
        # @param [Float] longitude
        # @param [Float] latitude
        # @param [Float] distance
        def initialize(longitude, latitude, distance)
          super()
          @longitude = longitude
          @latitude = latitude
          @distance = distance
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {
            "location" => [@longitude, @latitude],
            "distance" => @distance,
          }
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {GeoBoundingBoxQuery} body
      #
      # @yieldparam [GeoDistanceQuery] query
      #
      # @param [Float] top_left_longitude
      # @param [Float] top_left_latitude
      # @param [Float] bottom_right_longitude
      # @param [Float] bottom_right_latitude
      #
      # @return [GeoBoundingBoxQuery]
      def self.geo_bounding_box(top_left_longitude, top_left_latitude, bottom_right_longitude, bottom_right_latitude, &block)
        GeoBoundingBoxQuery.new(top_left_longitude, top_left_latitude, bottom_right_longitude, bottom_right_latitude, &block)
      end

      # Finds `geopoint` indexed matches in a given bounding box.
      class GeoBoundingBoxQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @yieldparam [GeoBoundingBoxQuery] self
        #
        # @param [Float] top_left_longitude
        # @param [Float] top_left_latitude
        # @param [Float] bottom_right_longitude
        # @param [Float] bottom_right_latitude
        def initialize(top_left_longitude, top_left_latitude, bottom_right_longitude, bottom_right_latitude)
          super()
          @top_left_longitude = top_left_longitude
          @top_left_latitude = top_left_latitude
          @bottom_right_longitude = bottom_right_longitude
          @bottom_right_latitude = bottom_right_latitude
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {
            "top_left" => [@top_left_longitude, @top_left_latitude],
            "bottom_right" => [@bottom_right_longitude, @bottom_right_latitude],
          }
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {ConjunctionQuery} body
      #
      # @yieldparam [ConjunctionQuery] query
      #
      # @return [ConjunctionQuery]
      def self.conjuncts(*queries, &block)
        ConjunctionQuery.new(*queries, &block)
      end

      # Result documents must satisfy all of the child queries.
      class ConjunctionQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @yieldparam [ConjunctionQuery] self
        #
        # @param [*SearchQuery] queries
        def initialize(*queries)
          super()
          @queries = queries.flatten
          yield self if block_given?
        end

        # @param [*SearchQuery] queries
        def and_also(*queries)
          @queries |= queries.flatten
        end

        def empty?
          @queries.empty?
        end

        # @return [String]
        def to_json(*args)
          raise ArgumentError, "compound conjunction query must have sub-queries" if @queries.nil? || @queries.empty?

          data = {"conjuncts" => @queries.uniq}
          data["boost"] = boost if boost
          data.to_json(*args)
        end
      end

      # Prepare {ConjunctionQuery} body
      #
      # @yieldparam [ConjunctionQuery] query
      #
      # @return [ConjunctionQuery]
      def self.disjuncts(*queries, &block)
        DisjunctionQuery.new(*queries, &block)
      end

      # Result documents must satisfy a configurable min number of child queries.
      class DisjunctionQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [Integer]
        attr_accessor :min

        # @yieldparam [DisjunctionQuery] self
        #
        # @param [*SearchQuery] queries
        def initialize(*queries)
          super()
          @queries = queries.flatten
          yield self if block_given?
        end

        # @param [*SearchQuery] queries
        def or_else(*queries)
          @queries |= queries.flatten
        end

        def empty?
          @queries.empty?
        end

        # @return [String]
        def to_json(*args)
          raise ArgumentError, "compound disjunction query must have sub-queries" if @queries.nil? || @queries.empty?

          data = {"disjuncts" => @queries.uniq}
          if min
            raise ArgumentError, "disjunction query has fewer sub-queries than configured minimum" if @queries.size < min

            data["min"] = min
          end
          data["boost"] = boost if boost
          data.to_json(*args)
        end
      end

      # Prepare {BooleanQuery} body
      #
      # @yieldparam [BooleanQuery] query
      #
      # @return [BooleanQuery]
      def self.booleans(&block)
        BooleanQuery.new(&block)
      end

      # The boolean query is a useful combination of conjunction and disjunction queries.
      class BooleanQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @yieldparam [BooleanQuery] self
        def initialize
          super()
          @must = ConjunctionQuery.new
          @must_not = DisjunctionQuery.new
          @should = DisjunctionQuery.new
          yield self if block_given?
        end

        # @param [Integer] min minimal value for "should" disjunction query
        def should_min(min)
          @should.min = min
          self
        end

        # @param [*SearchQuery] queries
        def must(*queries)
          @must.and_also(*queries)
          self
        end

        # @param [*SearchQuery] queries
        def must_not(*queries)
          @must_not.or_else(*queries)
          self
        end

        # @param [*SearchQuery] queries
        def should(*queries)
          @should.or_else(*queries)
          self
        end

        # @return [String]
        def to_json(*args)
          if @must.empty? && @must_not.empty? && @should.empty?
            raise ArgumentError, "BooleanQuery must have at least one non-empty sub-query"
          end

          data = {}
          data["must"] = @must unless @must.empty?
          data["must_not"] = @must_not unless @must_not.empty?
          data["should"] = @should unless @should.empty?
          data["boost"] = boost if boost
          data.to_json(*args)
        end
      end

      # Prepare {TermQuery} body
      #
      # @yieldparam [TermQuery] query
      # @param [String] term
      #
      # @return [TermQuery]
      def self.term(term, &block)
        TermQuery.new(term, &block)
      end

      # A query that looks for **exact** matches of the term in the index (no analyzer, no stemming). Useful to check what the actual
      # content of the index is. It can also apply fuzziness on the term. Usual better alternative is `MatchQuery`.
      class TermQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @return [Integer]
        attr_accessor :fuzziness

        # @return [Integer]
        attr_accessor :prefix_length

        # @yieldparam [TermQuery] self
        #
        # @param [String] term
        def initialize(term)
          super()
          @term = term
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"term" => @term}
          data["boost"] = boost if boost
          data["field"] = field if field
          if fuzziness
            data["fuzziness"] = fuzziness
            data["prefix_length"] = prefix_length if prefix_length
          end
          data.to_json(*args)
        end
      end

      # Prepare {PrefixQuery} body
      #
      # @yieldparam [PrefixQuery] query
      # @param [String] prefix
      #
      # @return [PrefixQuery]
      def self.prefix(prefix, &block)
        PrefixQuery.new(prefix, &block)
      end

      # The prefix query finds documents containing terms that start with the provided prefix. Usual better alternative is `MatchQuery`.
      class PrefixQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @yieldparam [PrefixQuery] self
        #
        # @param [String] prefix
        def initialize(prefix)
          super()
          @prefix = prefix
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"prefix" => @prefix}
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {PhraseQuery} body
      #
      # @yieldparam [PhraseQuery] query
      # @param [*String] terms
      #
      # @return [PhraseQuery]
      def self.phrase(*terms, &block)
        PhraseQuery.new(*terms, &block)
      end

      # A query that looks for **exact** match of several terms (in the exact order) in the index. Usual better alternative is
      # {MatchPhraseQuery}.
      class PhraseQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @return [String]
        attr_accessor :field

        # @yieldparam [PhraseQuery] self
        #
        # @param [*String] terms
        def initialize(*terms)
          super()
          @terms = terms.flatten
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"terms" => @terms.flatten.uniq}
          data["boost"] = boost if boost
          data["field"] = field if field
          data.to_json(*args)
        end
      end

      # Prepare {MatchAllQuery} body
      #
      # @yieldparam [MatchAllQuery] query
      #
      # @return [MatchAllQuery]
      def self.match_all(&block)
        MatchAllQuery.new(&block)
      end

      # A query that matches all indexed documents.
      class MatchAllQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @yieldparam [MatchAllQuery] self
        def initialize
          super()
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"match_all" => nil}
          data["boost"] = boost if boost
          data.to_json(*args)
        end
      end

      # Prepare {MatchNoneQuery} body
      #
      # @yieldparam [MatchNoneQuery] query
      #
      # @return [MatchNoneQuery]
      def self.match_none(&block)
        MatchNoneQuery.new(&block)
      end

      # A query that matches nothing.
      class MatchNoneQuery < SearchQuery
        # @return [Float]
        attr_accessor :boost

        # @yieldparam [MatchNoneQuery] self
        def initialize
          super()
          yield self if block_given?
        end

        # @return [String]
        def to_json(*args)
          data = {"match_none" => nil}
          data["boost"] = boost if boost
          data.to_json(*args)
        end
      end
    end

    class SearchSort
      # @yieldparam [SearchSortScore]
      # @return [SearchSortScore]
      def self.score(&block)
        SearchSortScore.new(&block)
      end

      # @yieldparam [SearchSortId]
      # @return [SearchSortScore]
      def self.id(&block)
        SearchSortId.new(&block)
      end

      # @param [String] name field name
      # @yieldparam [SearchSortField]
      # @return [SearchSortField]
      def self.field(name, &block)
        SearchSortField.new(name, &block)
      end

      # @param [String] name field name
      # @param [Float] longitude
      # @param [Float] latitude
      # @yieldparam [SearchSortField]
      # @return [SearchSortGeoDistance]
      def self.geo_distance(name, longitude, latitude, &block)
        SearchSortGeoDistance.new(name, longitude, latitude, &block)
      end

      class SearchSortScore < SearchSort
        # @return [Boolean] if descending order should be applied
        attr_accessor :desc

        # @yieldparam [SearchSortScore]
        def initialize
          super
          yield self if block_given?
        end

        # @api private
        def to_json(*args)
          {by: :score, desc: desc}.to_json(*args)
        end
      end

      class SearchSortId < SearchSort
        # @return [Boolean] if descending order should be applied
        attr_accessor :desc

        # @yieldparam [SearchSortId]
        def initialize
          super
          yield self if block_given?
        end

        # @api private
        def to_json(*args)
          {by: :id, desc: desc}.to_json(*args)
        end
      end

      class SearchSortField < SearchSort
        # @return [String] name of the field to sort by
        attr_reader :field

        # @return [Boolean] if descending order should be applied
        attr_accessor :desc

        # @return [:auto, :string, :number, :date]
        attr_accessor :type

        # @return [:first, :last] where the documents with missing field should be placed
        attr_accessor :missing

        # @return [:default, :min, :max]
        attr_accessor :mode

        # @param [String] field the name of the filed for ordering
        # @yieldparam [SearchSortField]
        def initialize(field)
          super()
          @field = field
          yield self if block_given?
        end

        # @api private
        def to_json(*args)
          {by: :field, field: field, desc: desc, type: type, missing: missing, mode: mode}.to_json(*args)
        end
      end

      class SearchSortGeoDistance < SearchSort
        # @return [String] name of the field to sort by
        attr_reader :field

        # @return [Boolean] if descending order should be applied
        attr_accessor :desc

        # @return [Float]
        attr_reader :longitude

        # @return [Float]
        attr_reader :latitude

        # @return [:meters, :miles, :centimeters, :millimeters, :kilometers, :nauticalmiles, :feet, :yards, :inch]
        attr_accessor :unit

        # @param [String] field field name
        # @param [Float] longitude
        # @param [Float] latitude
        # @yieldparam [SearchSortGeoDistance]
        def initialize(field, longitude, latitude)
          super()
          @field = field
          @longitude = longitude
          @latitude = latitude
          yield self if block_given?
        end

        # @api private
        def to_json(*args)
          {by: :geo_distance, field: field, desc: desc, location: [longitude, latitude], unit: unit}.to_json(*args)
        end
      end
    end

    class SearchFacet
      # @param [String] field_name
      # @return [SearchFacetTerm]
      def self.term(field_name, &block)
        SearchFacetTerm.new(field_name, &block)
      end

      # @param [String] field_name
      # @return [SearchFacetNumericRange]
      def self.numeric_range(field_name, &block)
        SearchFacetNumericRange.new(field_name, &block)
      end

      # @param [String] field_name
      # @return [SearchFacetDateRange]
      def self.date_range(field_name, &block)
        SearchFacetDateRange.new(field_name, &block)
      end

      class SearchFacetTerm
        # @return [String]
        attr_reader :field

        # @return [Integer]
        attr_accessor :size

        # @param [String] field name of the field
        def initialize(field)
          @field = field
          yield self if block_given?
        end

        # @api private
        def to_json(*args)
          {field: field, size: size}.to_json(*args)
        end
      end

      class SearchFacetNumericRange
        # @return [String]
        attr_reader :field

        # @return [Integer]
        attr_accessor :size

        # @param [String] field name of the field
        def initialize(field)
          @field = field
          @ranges = []
          yield self if block_given?
        end

        # @param [String] name the name of the range
        # @param [Integer, Float, nil] min lower bound of the range (pass +nil+ if there is no lower bound)
        # @param [Integer, Float, nil] max upper bound of the range (pass +nil+ if there is no upper bound)
        def add(name, min, max)
          @ranges.append({name: name, min: min, max: max})
        end

        # @api private
        def to_json(*args)
          {field: field, size: size, numeric_ranges: @ranges}.to_json(*args)
        end
      end

      class SearchFacetDateRange
        # @return [String]
        attr_reader :field

        # @return [Integer]
        attr_accessor :size

        # @param [String] field name of the field
        def initialize(field)
          super()
          @field = field
          @ranges = []
          yield self if block_given?
        end

        DATE_FORMAT_RFC3339 = "%Y-%m-%dT%H:%M:%S%:z".freeze

        # @param [String] name the name of the range
        # @param [Time, String, nil] start_time lower bound of the range (pass +nil+ if there is no lower bound)
        # @param [Time, String, nil] end_time lower bound of the range (pass +nil+ if there is no lower bound)
        def add(name, start_time, end_time)
          start_time = start_time.strftime(DATE_FORMAT_RFC3339) if start_time.respond_to?(:strftime)
          end_time = end_time.strftime(DATE_FORMAT_RFC3339) if end_time.respond_to?(:strftime)
          @ranges.append({name: name, start: start_time, end: end_time})
        end

        # @api private
        def to_json(*args)
          {field: field, size: size, date_ranges: @ranges}.to_json(*args)
        end
      end
    end

    class SearchOptions
      # @return [Integer] Timeout in milliseconds
      attr_accessor :timeout

      # @return [Integer] limits the number of matches returned from the complete result set.
      attr_accessor :limit

      # @return [Integer] indicates how many matches are skipped on the result set before starting to return the matches
      attr_accessor :skip

      # @return [Boolean] triggers inclusion of additional search result score explanations. (Default: +false+)
      attr_accessor :explain

      # @return [:html, :ansi] the style of highlighting in the result excerpts (if not specified, the server default will be used)
      attr_accessor :highlight_style

      # @return [Array<String>] list of the fields to highlight
      attr_accessor :highlight_fields

      # @return [Array<String>] list of field values which should be retrieved for result documents, provided they were stored while
      #   indexing
      attr_accessor :fields

      # @return [:not_bounded] specifies level of consistency for the query
      attr_reader :scan_consistency

      # @api private
      # @return [MutationState]
      attr_reader :mutation_state

      # Customizes the consistency guarantees for this query
      #
      # @note overrides consistency level set by {#consistent_with}
      #
      # [+:not_bounded+] The engine will return whatever state it has at the time of query
      #
      # @param [:not_bounded] level the scan consistency to be used for this query
      #
      # @return [void]
      def scan_consistency=(level)
        @mutation_state = nil if @mutation_state
        @scan_consistency = level
      end

      # Sets the mutation tokens this query should be consistent with
      #
      # @note overrides consistency level set by {#scan_consistency=}
      #
      # @param [MutationState] mutation_state the mutation state containing the mutation tokens
      #
      # @return [void]
      def consistent_with(mutation_state)
        @scan_consistency = nil if @scan_consistency
        @mutation_state = mutation_state
      end

      # Ordering rules to apply to the results
      #
      # The list might contain either strings or special objects, that derive from {SearchSort}.
      #
      # In case of String, the value represents the name of the field with optional +-+ in front of the name, which will turn on descending
      # mode for this field. One field is special is +"_score"+ which will sort results by their score.
      #
      # When nothing specified, the Server will order results by their score descending, which is equivalent of +"-_score"+.
      #
      # @return [Array<String, SearchSort>] list of ordering object
      attr_accessor :sort

      # Facets allow to aggregate information collected on a particular result set
      #
      # @return [Hash<String => SearchFacet>]
      attr_accessor :facets

      # @return [JsonTranscoder] transcoder to use for the results
      attr_accessor :transcoder

      # @yieldparam [SearchOptions] self
      def initialize
        super
        @explain = false
        @transcoder = JsonTranscoder.new
        @scan_consistency = nil
        @mutation_state = nil
        yield self if block_given?
      end
    end

    class SearchRowLocation
      # @return [String]
      attr_accessor :field

      # @return [String]
      attr_accessor :term

      # @return [Integer] the position of the term within the field, starting at 1
      attr_accessor :position

      # @return [Integer] start byte offset of the term in the field
      attr_accessor :start_offset

      # @return [Integer] end byte offset of the term in the field
      attr_accessor :end_offset

      # @return [Array<Integer>] the positions of the term within any elements.
      attr_accessor :array_positions
    end

    class SearchRowLocations
      # Lists all locations (any field, any term)
      #
      # @return [Array<SearchRowLocation>]
      def get_all
        @locations
      end

      # Lists all locations for a given field (any term)
      #
      # @return [Array<SearchRowLocation>]
      def get_for_field(name)
        @locations.select { |location| location.field == name }
      end

      # Lists all locations for a given field and term
      #
      # @return [Array<SearchRowLocation>]
      def get_for_field_and_term(name, term)
        @locations.select { |location| location.field == name && location.term == term }
      end

      # Lists the fields in this location
      #
      # @return [Array<String>]
      def fields
        @locations.map(&:field).uniq
      end

      # Lists all terms in this locations, considering all fields
      #
      # @return [Array<String>]
      def terms
        @locations.map(&:term).uniq
      end

      # Lists the terms for a given field
      #
      # @return [Array<String>]
      def terms_for_field(name)
        get_for_field(name).map(&:term).uniq
      end

      # @param [Array<SearchRowLocation>] locations
      def initialize(locations)
        super()
        @locations = locations
      end
    end

    # An individual facet result has both metadata and details, as each facet can define ranges into which results are categorized
    class SearchFacetResult
      # @return [String]
      attr_accessor :name

      # @return [String]
      attr_accessor :field

      # @return [Integer]
      attr_accessor :total

      # @return [Integer]
      attr_accessor :missing

      # @return [Integer]
      attr_accessor :other

      class TermFacetResult < SearchFacetResult
        # @return [Array<TermFacet>]
        attr_accessor :terms

        def type
          :term_facet
        end

        # @yieldparam [TermFacetResult] self
        def initialize
          super
          yield self if block_given?
        end

        class TermFacet
          # @return [String]
          attr_reader :term

          # @return [Integer]
          attr_reader :count

          def initialize(term, count)
            super()
            @term = term
            @count = count
          end
        end
      end

      class DateRangeFacetResult < SearchFacetResult
        # @return [Array<DateRangeFacet>]
        attr_accessor :date_ranges

        def type
          :date_range_facet
        end

        # @yieldparam [DateRangeFacetResult] self
        def initialize
          super
          yield self if block_given?
        end

        class DateRangeFacet
          # @return [String]
          attr_reader :name

          # @return [Integer]
          attr_reader :count

          # @return [String]
          attr_reader :start_time

          # @return [String]
          attr_reader :end_time

          def initialize(name, count, start_time, end_time)
            @name = name
            @count = count
            @start_time = start_time
            @end_time = end_time
          end
        end
      end

      class NumericRangeFacetResult < SearchFacetResult
        # @return [Array<NumericRangeFacet>]
        attr_accessor :numeric_ranges

        def type
          :numeric_range_facet
        end

        # @yieldparam [NumericRangeFacetResult] self
        def initialize
          super
          yield self if block_given?
        end

        class NumericRangeFacet
          # @return [String]
          attr_reader :name

          # @return [Integer]
          attr_reader :count

          # @return [Integer, Float, nil]
          attr_reader :min

          # @return [Integer, Float, nil]
          attr_reader :max

          def initialize(name, count, min, max)
            @name = name
            @count = count
            @min = min
            @max = max
          end
        end
      end
    end

    class SearchRow
      # @return [String] name of the index
      attr_accessor :index

      # @return [String] document identifier
      attr_accessor :id

      # @return [Float]
      attr_accessor :score

      # @return [SearchRowLocations]
      attr_accessor :locations

      # @return [Hash]
      attr_accessor :explanation

      # @return [Hash<String => Array<String>>]
      attr_accessor :fragments

      # @return [JsonTranscoder] transcoder to use for the fields
      attr_accessor :transcoder

      def fields
        @transcoder.decode(@fields, :json) if @fields && @transcoder
      end

      # @yieldparam [SearchRow] self
      def initialize
        @fields = nil
        yield self if block_given?
      end
    end

    class SearchMetrics
      # @return [Integer] time spent executing the query (in milliseconds)
      attr_accessor :took

      # @return [Integer]
      attr_accessor :total_rows

      # @return [Float]
      attr_accessor :max_score

      # @return [Integer]
      attr_accessor :success_partition_count

      # @return [Integer]
      attr_accessor :error_partition_count

      # @return [Integer]
      def total_partition_count
        success_partition_count + error_partition_count
      end
    end

    class SearchMetaData
      # @return [SearchMetrics]
      attr_accessor :metrics

      # @return [Hash<String => String>]
      attr_accessor :errors

      # @yieldparam [SearchMetaData] self
      def initialize
        @metrics = SearchMetrics.new
        yield self if block_given?
      end
    end

    class SearchResult
      # @return [Array<SearchRow>]
      attr_accessor :rows

      # @return [Hash<String => SearchFacetResult>]
      attr_accessor :facets

      # @return [SearchMetaData]
      attr_accessor :meta_data

      # @yieldparam [SearchResult] self
      def initialize
        yield self if block_given?
      end
    end
  end
end
