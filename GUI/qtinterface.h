#ifndef QTINTERFACE_H
#define QTINTERFACE_H

#include <iostream>
#include <sstream>

#include <QObject>
#include <QThread>

#include "Framework/Types.hpp"
#include "Framework/BreakpointList.hpp"
#include "Framework/StatInfo.hpp"
#include "Framework/RegInfo.hpp"
#include "Framework/Tools.hpp"

#include "qtinterfacetypes.h"
#include "mainwindow.h"

class BasicCPU;
class BasicDeviceRegistry;
class BasicLoader;
class BreakpointList;
class MainWindow;
class QtInterface;

class RunModeWorker : public QObject
{
    Q_OBJECT
    void run();

private:
    bool running = false;
    bool stepping = false;
    QtInterface* interface;
public:
    RunModeWorker(QtInterface* interface, QObject * parent = 0);

public slots:
    void startRunning();
    void startStepping();
    void stopRunning();
signals:
    void stoppedRunning(QString reason);
    void stepped();
};

class QtInterface : public QObject
{
  Q_OBJECT
  friend class RunModeWorker;

signals:
  void registerChanged(std::string name, std::string value);
  void programLoaded(const std::string fileName);
  void stepped();
  void startedRunning();
  void stoppedRunning();
  void startRunning();
  void startStepping();

public:
  QtInterface(BasicCPU &cpu, BasicDeviceRegistry &registry, BasicLoader &loader, MainWindow &window, std::ostream &buf = std::cout);
  ~QtInterface();
  std::stringstream* GetBuffer();
  void Log(std::string str);
  void LogLn(std::string str);
  void Trace(std::string str);
  void TraceLn(std::string str);

private:
  MainWindow &window;
  BasicCPU &myCPU;
  BasicDeviceRegistry &myDeviceRegistry;
  BasicLoader &myLoader;
  std::ostream &myOutputStream;
  BreakpointList &myBreakpointList;
  struct CommandTable {
    std::string name;
    void (QtInterface::*mfp)(const std::string &);
  };
  static CommandTable ourCommandTable[];
  const unsigned int myNumberOfCommands;
  std::stringstream* outBuffer;
  RunModeWorker* runModeWorker;
  QThread runModeThread;
  void outputChanged();

public:
  bool NewBreakpoint(uint32_t address);
  bool AttachDevice(std::string name, std::string deviceArgs, size_t addressSpace);
  void ClearStatistics();
  bool RemoveBreakpoint(uint32_t address);
  bool DetachDevice(size_t addressSpace, size_t deviceIndex);
  bool FillMemoryBlock(unsigned int addressSpace, std::string addressStr, unsigned long length, std::string value);
  void GetRegisters(QList<RegisterModel> &reglist);
  void ListAttachedDevices(unsigned int addressSpace);
  void ListBreakpoints();
  void ListDevices();
  std::string ListDeviceScript(std::string name);
  std::string ListExecutionTraceRecord();
  std::string ListDefaultExecutionTraceEntries();
  const unsigned int ListGranularity();
  std::string ListMemory(unsigned int addressSpace, std::string addressStr, unsigned int length, unsigned int wordsPerLine);
  uint32_t GetMaximumAddress(size_t addressSpace);
  size_t ListNumberOfAddressSpaces();
  RegisterModelList ListRegisters(RegisterModelList* list);
  std::string ListRegisterValue(std::string name);
  std::string ListRegisterDescription(std::string name);
  void ListStatistics();
  bool LoadProgram(const std::string &filename, int addressSpace);
  std::string ProgramCounterValue();
  void Reset();
  void Run();
  void RunStepping();
  bool SetRegister(std::string name, std::string value);
  void SetMemory(size_t addressSpace, std::string address, std::string value);
  void Step(int numberOfSteps);

  // Old interface command support
  void ExecuteCommand(const std::string &command);
  void AddBreakpoint(const std::string &args);
  void AttachDevice(const std::string &args);
  void ClearStatistics(const std::string &args);
  void DeleteBreakpoint(const std::string &args);
  void DetachDevice(const std::string &args);
  void FillMemoryBlock(const std::string &args);
  void ListAttachedDevices(const std::string &args);
  void ListBreakpoints(const std::string &args);
  void ListDevices(const std::string &args);
  void ListDeviceScript(const std::string &args);
  void ListExecutionTraceRecord(const std::string &args);
  void ListDefaultExecutionTraceEntries(const std::string &args);
  void ListGranularity(const std::string &args);
  void ListMemory(const std::string &args);
  void ListMaximumAddress(const std::string &args);
  void ListNumberOfAddressSpaces(const std::string &args);
  void ListRegisters(const std::string &args);
  void ListRegisterValue(const std::string &args);
  void ListRegisterDescription(const std::string &args);
  void ListStatistics(const std::string &args);
  void LoadProgram(const std::string &args);
  void ProgramCounterValue(const std::string &args);
  void Reset(const std::string &args);
  void Run(const std::string &args);
  void SetRegister(const std::string &args);
  void SetMemory(const std::string &args);
  void Step(const std::string &args);
};

#endif // QTINTERFACE_H
