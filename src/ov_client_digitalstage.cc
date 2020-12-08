#include "ov_client_digitalstage.h"
#include <iostream>
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

ov_client_digitalstage_t::ov_client_digitalstage_t(
    ov_render_base_t& backend, const std::string& frontend_url_, boost::filesystem::path& selfpath)
    : ov_client_base_t(backend), runservice(true), frontend_url(frontend_url_),
      quitrequest_(false)
{
  selfpath=selfpath.remove_filename();
  selfpath=selfpath.append("ds-config");
  ds_config_path=selfpath;

  std::cout<<"selfpath "  << selfpath <<   std::endl;
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
  }


  std::string url_= "https://auth.digital-stage.org/login?email=performerstone@gmail.com&password=L2eYaTD8dHpnwFe";
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
      std::cout<<"jwt : "<< str << std::endl;


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
