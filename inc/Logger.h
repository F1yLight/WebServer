#ifndef LOGGER_H
#define LOGGER_H

class Logger {
  private:
    static Logger *instance;
    
    Logger();
    ~Logger();

  public:
    static Logger *get_instance();
    void log(const char *format, ...);
};

#endif