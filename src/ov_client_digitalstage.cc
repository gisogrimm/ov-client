#include "ov_client_digitalstage.h"
#include <boost/filesystem.hpp>
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
#include <cpprest/ws_client.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

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

// auth data
std::string email;
std::string password;
std::string jwt;

nlohmann::json user;  //jsonObject for user data
nlohmann::json stage; //jsonObject for joined d-s stage

task_completion_event<void> tce; // used to terminate async PPLX listening task
websocket_callback_client wsclient;

ov_client_digitalstage_t::ov_client_digitalstage_t(
    ov_render_base_t& backend, const std::string& frontend_url_,
    boost::filesystem::path& selfpath)
    : ov_client_base_t(backend), runservice(true), frontend_url(frontend_url_),
      quitrequest_(false)
{
  std::cout << "app path " << selfpath << std::endl;
  selfpath = selfpath.remove_filename();
  selfpath = selfpath.append("ds-config");
  ds_config_path = selfpath;

  std::cout << "ds-config file path " << selfpath << std::endl;
  if(boost::filesystem::exists(selfpath)) {
    std::cout << "digital-stage config file found " << std::endl;
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
  tce.set(); // task completion event is set closing wss listening task
  wsclient.close(); // wss client is closed
  servicethread.join(); // thread is joined
}

void ov_client_digitalstage_t::service()
{
  // register_device(lobby, backend.get_deviceid());
  // download_file(lobby + "/announce.flac", "announce.flac");
  // start main control loop:

  ucout << "CHECK - digital-stage config file " << std::endl;

  if(boost::filesystem::exists("ds-config")) {
    std::cout << "OK - digital-stage config file FOUND  " << std::endl;

    std::string line;
    std::ifstream myfile("ds-config");

    if(myfile.is_open()) {
      std::getline(myfile, email);
      std::getline(myfile, password);
      // std::cout << email << "   " << password << '\n';

      myfile.close();
      if(email == "email") {
        std::cout << "Please provide email in ds-config file line 0" << '\n';
        quitrequest_ = true;
        return;
      }
      if(password == "password") {
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

  std::string url_ = "https://auth.digital-stage.org/login?email=" + email +
                     "&password=" + password;
  http_client client1(utility::conversions::to_string_t(url_));
  http_request request;
  request.set_method(methods::POST);
  request.headers().add(U("Host"), U("auth.digital-stage.org"));
  // request.headers().add(U("Origin"), U("https://test.digital-stage.org"));
  request.headers().add(U("Content-Type"), U("application/json"));

  client1.request(request)
      .then([](http_response response) {
        // std::cout<<"Response code is : "<<response.status_code();
        // std::cout<<"Response body is : "<<response.body();

        std::string str = response.extract_json().get().as_string();

        str.substr(1, str.length() - 1);
        jwt = str;
        std::cout << "jwt : " << jwt << std::endl;
      })
      .wait();

  wsclient.connect(U("wss://api.digital-stage.org")).wait();

  auto receive_task = create_task(tce);


  // handler for incoming d-s messages
  wsclient.set_message_handler([&](websocket_incoming_message ret_msg) {

// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----    parse incoming message events    -----
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------

    auto ret_str = ret_msg.extract_string().get();
    // ucout << "ret_str " << to_string_t(ret_str) << "\n";

    // we check if it's valid json
    //if(!nlohmann::json::accept(ret_str)) {
    //  std::cerr << "parse error" << std::endl;
    //}

    if(ret_str == "hey") {
      ucout << "ret_str " << to_string_t(ret_str) << "\n";
    } else {
      try {
        nlohmann::json j = nlohmann::json::parse(ret_str);
        ucout << "/----------------------Event--------------------------/"
                  << std::endl;
        ucout << j["data"].dump(4) << std::endl;

// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------
// -----    user ready    -----
// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------
// ----------------------------

        if (j["data"][0] == "user-ready")
        {
          ucout << "\n/--  USER_READY --/\n" << std::endl;
          ucout << j["data"][1].dump(4)      << std::endl;

          //we get user data from incoming json event message
          //and put the data into user json object for later use
          user = j["data"][1];
          // we can get single json entry fron nlohmann::json like this:
          ucout << "_id:  " << user["_id"]   << std::endl;

          // or we can dump nlohmann::json objects like this:
          ucout << "user:  " << user.dump(4) << std::endl;

        }

// ------------------------------
// ------------------------------
// ------------------------------
// ------------------------------
// ------------------------------
// ------------------------------
// -----    stage joined    -----
// ------------------------------
// ------------------------------
// ------------------------------
// ------------------------------
// ------------------------------
// ------------------------------

        if (j["data"][0] == "stage-joined" )
        {
          ucout << "\n/--  STAGE_JOINED --/\n" << std::endl;
          // put stage data into our stage jsonObject
          stage = j["data"][1];
          // print our stage
          ucout << "STAGE:\n" << stage.dump(4) << std::endl;
        }

// ------------------------------------------
// ------------------------------------------
// ------------------------------------------
// ------------------------------------------
// ------------------------------------------
// ------------------------------------------
// -----    stage member audio added    -----
// ------------------------------------------
// ------------------------------------------
// ------------------------------------------
// ------------------------------------------
// ------------------------------------------
// ------------------------------------------

        if(j["data"] == "stage-member-audio-added") {
          ucout << "/------------  STAGE_MEMBER_AUDIO_ADDED_EVENT   "
                       "--------------------------/"
                    << std::endl;
        }


      }
      catch( const std::exception& e) {
        std::cerr << "std::exception: " << e.what() << std::endl;
      }
      catch(...) {
        std::cerr << "error parsing" << std::endl;
      }
    }


  });

  utility::string_t close_reason;
  wsclient.set_close_handler([&close_reason](websocket_close_status status,
                                             const utility::string_t& reason,
                                             const std::error_code& code) {
    ucout << " closing reason..." << reason << "\n";
    ucout << "connection closed, reason: " << reason
          << " close status: " << int(status) << " error code " << code
          << std::endl;
  });

  nlohmann::json token_json;

  std::string body_str("{\"type\":0,\"data\":[\"token\",{\"token\":\"" + jwt +
                       "\"}]}");
  websocket_outgoing_message msg;
  msg.set_utf8_message(body_str);
  wsclient.send(msg).wait();
  receive_task.wait();

  // this part is never reached as receive_task.wait() is blocking the thread
  // until ov_client_digitalstage_t::stop_service() is called
  // this part is for reference only ! @Giso we can delete the while loop
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
