#include <httplib.h>

// Includes for registering nodes/ controllers
#include <crate/registrar/helper.hpp>
#include <crate/registrar/controller_v1.hpp>
#include <crate/registrar/node_v1.hpp>

// Includes for making and submitting readings
#include <crate/metrics/helper.hpp>
#include <crate/metrics/reading_v1.hpp>

#include <string>
#include <vector>
#include <thread>
#include <iostream>

namespace {

struct config_s {
   std::string url{"google.com"};
   uint64_t time{60};
   std::string monlith_ip{"0.0.0.0"};
   uint64_t monolith_port{9081};
};

config_s config;

int64_t get_timestamp() {
   return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()
   ).count();
}

void show_help() {

   auto help = R"(

short    long              description                            default
--------------------------------------------------------------------------
-h       --help            show this message                            NA
-u       --url             site to check connection             google.com
-t       --time            time interval in seconds to reach            60
-m       --monolith_ip     monolith ip                             0.0.0.0
-p       --monolith_port   monolith port                              9081

)";

   std::cout << help << std::endl;
}

bool attempt_connection() {

   std::cout << "attempting connection .... ";

   httplib::Client http_client(config.url, 80);

   auto result = http_client.Get("/");
   if (!result || (result.error() != httplib::Error::Success)) {
      std::cout << "failure\n";
      return false;
   }

   std::cout << "success\n";
   return true;
}

}

int main(int argc, char **argv) {

   // Check the arguments
   //
   std::vector<std::string> args(argv+1, argv+argc);

   for(std::size_t i = 0; i < args.size(); i++) {

      if (args[i] == "-h" || args[i] == "--help") {
         show_help();
         return 0;
      }

      if (args[i] == "-u" || args[i] == "--url") {
         if (i + 1 >= args.size()) {
            std::cerr << "--url (-u) command demands a parameter" << std::endl;
            return 1;
         }

         i++;
         config.url = args[i];
         continue;
      }

      if (args[i] == "-t" || args[i] == "--time") {
         if (i + 1 >= args.size()) {
            std::cerr << "--time (-t) command demands a parameter" << std::endl;
            return 1;
         }

         i++;

         try {
            config.time = std::stoull(args[i]);
         } catch (...) {
            std::cerr << "Failed to convert argument to uint64_t: " << args[i] << std::endl;
            return 1;
         }
         continue;
      }

      if (args[i] == "-m" || args[i] == "--monolith_ip") {
         if (i + 1 >= args.size()) {
            std::cerr << "--monolith_ip (-m) command demands a parameter" << std::endl;
            return 1;
         }

         i++;
         config.monlith_ip = args[i];
         continue;
      }

      if (args[i] == "-p" || args[i] == "--monolith_port") {
         if (i + 1 >= args.size()) {
            std::cerr << "--monolith_port (--p) command demands a parameter" << std::endl;
            return 1;
         }

         i++;

         try {
            config.monolith_port = std::stoull(args[i]);
         } catch (...) {
            std::cerr << "Failed to convert argument to uint64_t: " << args[i] << std::endl;
            return 1;
         }
         continue;
      }

   }

   // Ensure that the time is something valid
   //
   if (config.time == 0) {
      std::cout << "Config time was set to 0 -> Forcing the default of 60" << std::endl;
   }

   // Do the initial connection attempt to ensure that the config is valud
   //
   if (!attempt_connection()) {
      std::cerr << "Unable to reach configured http site at port 80: " << config.url << std::endl;
   } else {
      std::cout << "> Successfully connected to given site: " << config.url << std::endl;
   }

   // Indicate to the user the time that we will wait between connects
   //
   std::cout << "Checking site connection every " << config.time << " seconds" << std::endl;

   // Setup the local application as a node, with a single sensor (connection status)
   //
   crate::registrar::node_v1_c application_node("check-connect");

   // Create a sensor for the node that checks a url connection
   //
   crate::registrar::node_v1_c::sensor v1_sensor;
   v1_sensor.id = "check-connect-status";
   v1_sensor.description = "Indicates if the check-connect program could reach a given url";
   v1_sensor.type = "custom";
  
   // Add the senosr to the application_node
   //
   if (!application_node.add_sensor(v1_sensor)) {
      std::cerr << "\nFailed to add sensor : " << v1_sensor.id << std::endl;
      std::exit(1);
   }

   // Register the node/ sensor with the monolith
   //
   crate::registrar::helper_c registrar_helper(config.monlith_ip, config.monolith_port);
   if (registrar_helper.submit(application_node) != crate::registrar::helper_c::result::SUCCESS) {
      std::cerr << "\nFailed to register node with registrar" << std::endl;
      std::exit(1);
   }

   // Create a metrics helper so we can submit metrics
   //
   crate::metrics::helper_c metric_helper(
      crate::metrics::helper_c::endpoint_type_e::HTTP,
      config.monlith_ip,
      config.monolith_port
   );

   uint64_t failures{0};

   // Forever!
   //
   while(1) {
      // Obtain a "reading" for the connection "sensor"
      //
      auto reading = crate::metrics::sensor_reading_v1_c(
         get_timestamp(),
         "check-connect",
         "check-connect-status",
         static_cast<double>(attempt_connection())
      );

      // Submit the reading
      //
      if (metric_helper.submit(reading)  != crate::metrics::helper_c::result::SUCCESS) {
         std::cerr << "Failed to write metric ... failure #" << ++failures << std::endl;
      }

      std::this_thread::sleep_for(std::chrono::seconds(config.time));
   }

   return 0;
}
