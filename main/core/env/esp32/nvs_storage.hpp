#include "nvs_handle.hpp"
#include "storage.hpp"

using handle_t = std::unique_ptr<nvs::NVSHandle>;

class NvsStorage : public Storage {
public:
  Esp_Err_t save_durations(all_durations_t &arg) override;
  all_durations_t read_durations() const override;
  Esp_Err_t init() override;

private:
  const char *const handle_name{"storage"};
  handle_t handle{};
};
