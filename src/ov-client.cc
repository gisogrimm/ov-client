#include "ov_client_orlandoviols.h"
#include "ov_render_tascar.h"
#include <errmsg.h>
#include <stdint.h>
#include <string>
#include <udpsocket.h>


#include <cpprest/ws_client.h>
#include <nlohmann/json.hpp>

using namespace utility::conversions;
using namespace web::websockets::client;
using namespace pplx;

using json = nlohmann::json;

static bool quit_app(false);

static void sighandler(int sig)
{
  quit_app = true;
}

int main(int argc, char** argv)
{
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);

  websocket_callback_client client;
    client.connect(U("wss://api.digital-stage.org")).wait();

    task_completion_event<void> tce;
    auto receive_task = create_task(tce);

    client.set_message_handler([&](websocket_incoming_message ret_msg) {
        auto ret_str = ret_msg.extract_string().get();
        //ucout << "ret_str " << to_string_t(ret_str) << "\n";

        //we check if it's valid json
        if (!json::accept(ret_str))
        {
            std::cerr << "parse error" << std::endl;
        }

        if (ret_str == "hey")
        {
            ucout << "ret_str " << to_string_t(ret_str) << "\n";
        } else {
            try {
            json j = json::parse(ret_str);
            std::cout << "/----------------------Event--------------------------/" << std::endl;
            std::cout << j["data"].dump(4) << std::endl;

            //switch (j["data"])
            //{
            //case "stage-member-audio-added":
            //    std::cout << j["data"]["stage-member-audio-added"].dump(4) << std::endl;
            //    break;

            //default:
            //    break;
            //}

            } catch (...) {
            std::cerr << "error parsing file" << std::endl;
            }
        }



        //json j = json::parse(ret_str);
        //std::cout << j.dump(4) << std::endl;
        //json payload = ;
        //switch (payload[])
        //{
        //case /* constant-expression */:
        //    /* code */
        //    break;

        //default:
        //    break;
        //}

        //tce.set(); // this closes the task and fire client.close event
    });

    utility::string_t close_reason;
    client.set_close_handler([&close_reason](websocket_close_status status,
        const utility::string_t& reason,
        const std::error_code& code) {
        ucout << " closing reason..." << reason << "\n";
        ucout << "connection closed, reason: " << reason << " close status: " << int(status) << " error code " << code << std::endl;
    });

    json token_json;

    std::string body_str("{\"type\":0,\"data\":[\"token\",{\"token\":\"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyIjp7Il9pZCI6IjVmN2YwNjM0MDFkY2JkNzA4ZDZiYTg0MCIsImVtYWlsIjoicGVyZm9ybWVyc3RvbmVAZ21haWwuY29tIn0sImlhdCI6MTYwNjEyNzkwMSwiZXhwIjoxNjA2NzMyNzAxfQ.LJ-RDgRTqS0AFXphADRSOjuqhD8CTIb_Ov9mUEs8Ib4\"}]}");
    websocket_outgoing_message msg;
    msg.set_utf8_message(body_str);
    client.send(msg).wait();


  try {



    std::string deviceid(getmacaddr());
    std::string lobby("http://oldbox.orlandoviols.com/");
    int pinglogport(0);
    const char* options = "s:hqvd:p:";
    struct option long_options[] = {{"server", 1, 0, 's'},
                                    {"help", 0, 0, 'h'},
                                    {"quiet", 0, 0, 'q'},
                                    {"deviceid", 1, 0, 'd'},
                                    {"verbose", 0, 0, 'v'},
                                    {"pinglogport", 1, 0, 'p'},
                                    {0, 0, 0, 0}};
    int opt(0);
    int option_index(0);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case 'h':
        app_usage("ov-client", long_options, "");
        return 0;
      case 'q':
        verbose = 0;
        break;
      case 's':
        lobby = optarg;
        break;
      case 'd':
        deviceid = optarg;
        break;
      case 'p':
        pinglogport = atoi(optarg);
        break;
      case 'v':
        verbose++;
        break;
      }
    }
    if(deviceid.empty()) {
      throw ErrMsg("Invalid (empty) device id. Please ensure that the network "
                   "device is active or specify a valid device id.");
    }
    if(verbose)
      std::cout << "creating renderer with device id \"" << deviceid
                << "\" and pinglogport " << pinglogport << ".\n";
    ov_render_tascar_t render(deviceid, pinglogport);
    if(verbose)
      std::cout << "creating frontend interface for " << lobby << std::endl;
    ov_client_orlandoviols_t ovclient(render, lobby);
    if(verbose)
      std::cout << "starting services\n";
    ovclient.start_service();
    while(!quit_app) {
      receive_task.wait();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if(ovclient.quitrequest()) {
        quit_app = true;
      }
    }
    if(verbose)
      std::cout << "stopping services\n";
    ovclient.stop_service();
    client.close().wait();
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
