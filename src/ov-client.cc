class ov_render_base_t {
public:
  ov_render_base_t(){};
  virtual ~ov_render_base_t(){};
  void start_session(){};
  void end_session(){};
  void configure_audio_backend(const std::string& device, double srate,
                               unsigned int periodsize, unsigned int buffers){};
  void add_stage_device(stage_device_id_t stagedeviceid,
                        const std::string& name, unsigned int numchannels){};
  void rm_stage_device(stage_device_id_t stagedeviceid){};
  void set_stage_device_gain(stage_device_id_t stagedeviceid, double gain){};
};

class ov_client_base_t {
public:
  ov_client_base_t(ov_render_base_t& backend) : backend(backend){};
  virtual ~ov_client_base_t();
  virtual void run(){};

private:
  ov_render_base_t& backend;
};

int main(int argc, char** argv)
{
  return 0;
}
