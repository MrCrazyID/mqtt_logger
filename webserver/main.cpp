#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <signal.h>
#include <mongoose/Server.h>
#include <mongoose/WebController.h>
#include "../mqtt_logger/logger.hpp"
#include "../mqtt_logger/mqtt_logger.hpp"

// declarations
volatile bool receivedSIGINT{ false };
void handle_sigint (int);

using namespace std;
using namespace Mongoose;


class MyController : public WebController {
    public:
    MyController (mqtt_logger& logger)
    : _logger (logger) {
    }
    void hello (Request& request, StreamResponse& response) {
        response
        << "Hello "
        << htmlEntities (request.get ("name", "... what's your name ?")) << endl;
    }
    void get_data (Request& request, StreamResponse& response) {
        string data_request = request.get ("table", "message");
        response
        << "{\"nr_of_messages\": "
        << htmlEntities (to_string (_logger.get_message_count (data_request)))
        << "}" << endl;
    }

    void setup () {
        addRoute ("GET", "/hello", MyController, hello);
        addRoute ("GET", "/nr_of_messages", MyController, get_data);
    }

    private:
    mqtt_logger& _logger;
};


int main () {
    mqtt_logger MQTT_logger;
    // set signal handlers
    signal (SIGINT, handle_sigint);
    MyController myController (MQTT_logger);
    std::cout << "MQTT Webserver with logger" << std::endl;

    // init libmosquitto
    int mosquitto_lib_version[] = { 0, 0, 0 };
    mosqpp::lib_init ();
    mosqpp::lib_version (&mosquitto_lib_version[0], &mosquitto_lib_version[1],
    &mosquitto_lib_version[2]);
    std::cout << "using Mosquitto lib version " << mosquitto_lib_version[0]
              << '.' << mosquitto_lib_version[1] << '.'
              << mosquitto_lib_version[2] << std::endl;

    Server server (8080, "./website/");
    server.registerController (&myController);

    server.start ();

    try {

        while (!receivedSIGINT) {

            int rc = MQTT_logger.loop ();
            if (rc) {
                std::cout << "MQTT: attempting reconnect" << std::endl;
                MQTT_logger.reconnect ();
            }
        }
        std::cout << "Revieced signal for signalhandler" << std::endl;
    } catch (std::exception& e) {
        std::cout << "Exception " << e.what ();
    } catch (...) {
        std::cout << "UNKNOWN EXCEPTION" << std::endl;
    }

    std::cout << " MQTT Logger stopped" << std::endl;
    mosqpp::lib_cleanup ();
    return 0;
    /*while (1) {
#ifdef WIN32
        Sleep (10000);
#else
        sleep (10);
#endif
    }*/
}
// signal handlers
void handle_sigint (int) {
    receivedSIGINT = true;
}