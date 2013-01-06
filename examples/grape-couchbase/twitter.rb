# This example is lagerly based on grape README
# And is recreating parts of the Twitter API with Couchbase backend

require 'securerandom'

module Twitter
  class API < Grape::API
    version 'v1', :using => :header, :vendor => 'twitter'
    format :json

    helpers do
      include Rack::Stream::DSL

      def current_user
        @current_user ||= env['HTTP_X_USER']
      end

      def authenticate!
        error!('401 Unauthorized', 401) unless current_user
      end

      def next_id
        SecureRandom.urlsafe_base64(6)
      end

      def fetch_timeline(username = nil)
        view_params = {
          :include_docs => true,
          :limit => 20,
          :descending => true
        }
        if username
          view_name = "_design/twitter/_view/user_timeline"
          view_params[:start_key] = [username]
          view_params[:end_key] = ["#{username}\uefff"]
        else
          view_name = "_design/twitter/_view/public_timeline"
        end
        view = Couchbase::View.new(Couchbase.bucket, view_name, view_params)
        view.map do |row|
          {:id => row.id, :cas => row.meta['cas']}.merge(row.doc)
        end
      end
    end

    rescue_from :all do |ex|
      if ex.kind_of?(Couchbase::Error::Base)
        payload = {
          :error => ex.class.name.demodulize.underscore,
          :code => ex.error,
          :message => ex.message.sub(/ \(.*/, '')
        }
        payload[:key] = ex.key if ex.key
        case ex
        when Couchbase::Error::View
          payload[:view] = {
            :from => ex.from,
            :reason => ex.reason
          }
        when Couchbase::Error::HTTP
          payload[:http] = {
            :type => ex.type,
            :reason => ex.reason
          }
        end
        code = case ex
               when Couchbase::Error::NotFound
                 404
               when Couchbase::Error::KeyExists
                 409
               else
                 500
               end
        rack_response(MultiJson.dump(payload), code)
      else
        handle_error(ex)
      end
    end

    resource :statuses do

      desc "Return a public timeline."
      get :public_timeline do
        fetch_timeline
      end

      desc "Return a personal timeline."
      get :home_timeline do
        authenticate!
        fetch_timeline(current_user)
      end

      desc "Return a status."
      params do
        requires :id, :type => String, :desc => "Status ID."
      end
      get ':id' do
        val, _, cas = Couchbase.bucket.get(params[:id], :extended => true)
        {:ok => true, :cas => cas, :doc => val}
      end

      desc "Create a status."
      params do
        requires :status, :type => String, :desc => "Your status."
      end
      post do
        authenticate!
        data = {
          'type' => 'status',
          'user' => current_user,
          'text' => params[:status],
          'date' => DateTime.now.rfc2822
        }
        id = next_id
        cas = Couchbase.bucket.add(id, data)
        {:ok => true, :id => id, :cas => cas}
      end

      desc "Update a status."
      params do
        requires :id, :type => String, :desc => "Status ID."
        requires :status, :type => String, :desc => "Your status."
        optional :cas, :type => Bignum, :desc => "CAS value for optimistic lock"
      end
      put ':id' do
        authenticate!
        data = {
          'type' => 'status',
          'user' => current_user,
          'text' => params[:status],
          'date' => DateTime.now.rfc2822
        }
        cas = Couchbase.bucket.replace(params[:id], data, :cas => params[:cas])
        {:ok => true, :cas => cas}
      end

      desc "Delete a status."
      params do
        requires :id, :type => String, :desc => "Status ID."
        optional :cas, :type => Bignum, :desc => "CAS value for optimistic lock"
      end
      delete ':id' do
        authenticate!
        Couchbase.bucket.delete(params[:id], :cas => params[:cas])
        {:ok => true}
      end

    end
  end

end
