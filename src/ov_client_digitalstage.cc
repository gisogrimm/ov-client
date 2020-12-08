#include "ov_client_digitalstage.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cpprest/ws_client.h>
#include <cpprest/http_client.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/filestream.h>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>


using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace utility::conversions;
using namespace web::websockets::client;
using namespace pplx;
using namespace concurrency::streams;


using json = nlohmann::json;

boost::filesystem::path ds_config_path;
std::string email;
std::string password;
std::string jwt;

task_completion_event<void> tce;
websocket_callback_client wsclient;

ov_client_digitalstage_t::ov_client_digitalstage_t(
    ov_render_base_t& backend, const std::string& frontend_url_, boost::filesystem::path& selfpath)
    : ov_client_base_t(backend), runservice(true), frontend_url(frontend_url_),
      quitrequest_(false)
{
  std::cout<<"app path "  << selfpath <<   std::endl;
  selfpath=selfpath.remove_filename();
  selfpath=selfpath.append("ds-config");
  ds_config_path=selfpath;

  std::cout<<"ds-config file path "  << selfpath <<   std::endl;
  if (boost::filesystem::exists(selfpath))
  {
   std::cout<<"digital-stage config file found "  << std::endl;
  }
}

void ov_client_digitalstage_t::start_service()
{
  runservice = true;
  servicethread = std::thread(&ov_client_digitalstage_t::service, this);
}

void ov_client_digitalstage_t::stop_service()
{
  runservice = false;
  tce.set();
  wsclient.close();
  servicethread.join();
}

void ov_client_digitalstage_t::service()
{
  // register_device(lobby, backend.get_deviceid());
  // download_file(lobby + "/announce.flac", "announce.flac");
  // start main control loop:




  std::cout<<"CHECK - digital-stage config file "  << std::endl;


if (boost::filesystem::exists("ds-config"))
  {
    std::cout<<"OK - digital-stage config file FOUND  " << std::endl;

    std::string line;
    std::ifstream myfile ("ds-config");

    if (myfile.is_open())
    {
      std::getline(myfile, email);
      std::getline(myfile, password);
      //std::cout << email << "   " << password << '\n';

      myfile.close();
      if (email == "email")
      {
        std::cout << "Please provide email in ds-config file line 0" << '\n';
        quitrequest_ = true;
        return;
      }
      if (password == "password")
      {
        std::cout << "Please provide password in ds-config file line 1" << '\n';
        quitrequest_ = true;
        return;
      }
    }

    else {
      std::cout << "Unable to open ds-config file";
      quitrequest_ = true;
        return;
      }

  }

  std::string url_= "https://auth.digital-stage.org/login?email=" + email + "&password=" + password;
  http_client client1(utility::conversions::to_string_t(url_));
  http_request request;
  request.set_method(methods::POST);
  request.headers().add(U("Host"), U("auth.digital-stage.org"));
  //request.headers().add(U("Origin"), U("https://test.digital-stage.org"));
  request.headers().add(U("Content-Type"), U("application/json"));

  client1.request(request).then([](http_response response)
  {

      //std::cout<<"Response code is : "<<response.status_code();
      //std::cout<<"Response body is : "<<response.body();

      std::string str = response.extract_json().get().as_string();

      str.substr(1,str.length() - 1);
      jwt=str;
      std::cout<<"jwt : "<< jwt << std::endl;


  }).wait();


    wsclient.connect(U("wss://api.digital-stage.org")).wait();


    auto receive_task = create_task(tce);

    wsclient.set_message_handler([&](websocket_incoming_message ret_msg) {
        auto ret_str = ret_msg.extract_string().get();
        //ucout << "ret_str " << to_string_t(ret_str) << "\n";

        //we check if it's valid json
        if (!nlohmann::json::accept(ret_str))
        {
            std::cerr << "parse error" << std::endl;
        }

        if (ret_str == "hey")
        {
            ucout << "ret_str " << to_string_t(ret_str) << "\n";
        } else {
            try {
            nlohmann::json j = nlohmann::json::parse(ret_str);
            std::cout << "/----------------------Event--------------------------/" << std::endl;
            std::cout << j["data"].dump(4) << std::endl;

            if (j["data"] == "stage-member-audio-added")
            {
              std::cout << "/------------  STAGE_MEMBER_AUDIO_ADDED_EVENT   --------------------------/" << std::endl;
            }

            //switch (j["data"])
            //{
            //case "stage-member-audio-added":
            //    std::cout << j["data"]["stage-member-audio-added"].dump(4) << std::endl;
            //    break;

            //default:
            //    break;
            //}

            } catch (...) {
            std::cerr << "error parsing" << std::endl;
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
    wsclient.set_close_handler([&close_reason](websocket_close_status status,
        const utility::string_t& reason,
        const std::error_code& code) {
        ucout << " closing reason..." << reason << "\n";
        ucout << "connection closed, reason: " << reason << " close status: " << int(status) << " error code " << code << std::endl;
    });

    nlohmann::json token_json;

    std::string body_str("{\"type\":0,\"data\":[\"token\",{\"token\":\"" + jwt + "\"}]}");
    websocket_outgoing_message msg;
    msg.set_utf8_message(body_str);
    wsclient.send(msg).wait();
    receive_task.wait();

  while(runservice) {
    std::cerr << "Error: not yet implemented." << std::endl;
    quitrequest_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
