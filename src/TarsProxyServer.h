// Copyright (c) 2020
// Linux Foundation Projects
//       Link: https://www.linuxfoundation.org/projects/
// TARS Foundation Projects
//       Link: https://github.com/TarsCloud
// All rights reserved.

#ifndef _TarsProxyServer_H_
#define _TarsProxyServer_H_

#include <iostream>
#include "servant/Application.h"
#include "util/tc_thread_rwlock.h"

/**
 *
 **/
class TarsProxyServer : public Application {
 public:
  /**
       *
       **/
  virtual ~TarsProxyServer() {}

  /**
       *
       **/
  virtual void initialize();

  /**
       *
       **/
  virtual void destroyApp();

  string getProxyObj(const string &servant, bool &hasProxy);

  bool checkAuth(const string &ip, const string &obj, const string &func);

  // bool _forNode;

 protected:
  bool cmdLoadProxyConf(const string &command, const string &params, string &result);

  bool loadConf(bool isStartMain);

 private:
  bool _checkAuthOnOff;
  map<string, set<string> > _authList;  // key: obj:func,  value: ip set
  set<string> _ipWhiteList;
  set<string> _ipBlackList;
  set<string> _serverWhiteList;
  map<string, string> _proxy;

  TC_ThreadRWLocker _confRWLock;
};

extern TarsProxyServer g_app;


#endif