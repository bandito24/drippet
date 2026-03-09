
#include <iostream>
#include <sys/time.h>
#include <time.h>

void setTime(int year, int month, int day, int hour = 0, int min = 0,
             int second = 0) {
  struct tm tm;
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = second;
  time_t t = mktime(&tm);
  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);
}

int main() {
  setTime(2022, 12, 5, 12, 30);

  std::string tag = "mystring";
  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  time(&now);
  // Set timezone to China Standard Time

  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  std::string s = strftime_buf;

  printf("current time id %s", s.c_str());
  int i = 2;
  return 0;
}
