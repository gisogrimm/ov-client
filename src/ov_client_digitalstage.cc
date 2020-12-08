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

boost::filesystem::path ds_config_path;
std::string email;
std::string password;
std::string jwt;

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
