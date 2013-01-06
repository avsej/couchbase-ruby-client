$LOAD_PATH.unshift(File.dirname(__FILE__) + "/../../lib")
$LOAD_PATH.unshift(File.dirname(__FILE__))

require 'bundler'
Bundler.require

require 'couchbase'
require 'twitter'

puts <<EOM
Welcome to Couchbase/Grape demo!

This service understands following requests. You can pass parameters
either to query string or encode as JSON into the HTTP body. For
example:

  curl -vv -X POST -d'{"status":"mystatus"}' -H'X-User:foobar' http://localhost:9292/statuses.json

EOM

Twitter::API.routes.each do |rr|
  puts "\n#{rr.route_method} #{rr.route_path.sub(':format', 'json')}"
  puts "#{rr.route_description}"
  unless rr.route_params.empty?
    puts "--------+-----------+------------"
    puts " name   | mandatory | description"
    puts "--------+-----------+------------"
    rr.route_params.each do |name, info|
      printf " %-6s | %-9s | %s\n", name, info[:required], info[:desc]
    end
  end
end
puts

Couchbase.connection_options = {:bucket => "default", :host => "localhost"}
unless Couchbase.bucket.design_docs["twitter"]
  ddoc = {
    '_id' => '_design/twitter',
    'views' => {
      'public_timeline' => {
        'map' => <<-EOJ
          function(doc, meta) {
            if (doc.type == "status" && doc.date) {
              // consider that doc.date is valid input
              emit(new Date(doc.date))
            }
          }
        EOJ
      },
      'user_timeline' => {
        'map' => <<-EOJ
          function(doc, meta) {
            if (doc.type == "status" && doc.user && doc.date) {
              // consider that doc.date is valid input
              emit([doc.user, new Date(doc.date)])
            }
          }
        EOJ
      }
    }
  }
  Couchbase.bucket.save_design_doc(ddoc)
end
run Twitter::API
