#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>

//#include "Framework/Interface.hpp"
#include "qtinterface.h"
#include "Framework/BasicCPU.hpp"
#include "Framework/AddressSpace.hpp"
#include "Framework/BasicDeviceRegistry.hpp"
#include "Framework/BasicLoader.hpp"
#include "M68k/devices/M68681.hpp"


QtInterface::QtInterface(BasicCPU &cpu, BasicDeviceRegistry &registry,
                     BasicLoader &loader, MainWindow &window, std::ostream &buf)
    : myCPU(cpu), myDeviceRegistry(registry), myLoader(loader),
      myOutputStream(buf),
      myBreakpointList(*new BreakpointList),
      //myNumberOfCommands(sizeof(ourCommandTable) / sizeof(CommandTable)) //Compilation error in VS due to bug
      myNumberOfCommands(27),
      window(window), runModeWorker(new RunModeWorker(this)) {
  connect(runModeWorker,SIGNAL(stoppedRunning(QString)),&window,SIGNAL(stoppedRunning(QString)));
  connect(runModeWorker,SIGNAL(stepped()),&window,SIGNAL(stepped()), Qt::BlockingQueuedConnection);
  connect(&window,SIGNAL(stopRunning()),runModeWorker,SLOT(stopRunning()), Qt::DirectConnection);
  connect(this,SIGNAL(startedRunning()),&window,SIGNAL(startedRunning()));
  connect(this,SIGNAL(startRunning()),runModeWorker,SLOT(startRunning()));
  connect(this,SIGNAL(startStepping()),runModeWorker,SLOT(startStepping()));

  runModeWorker->moveToThread(&runModeThread);
  runModeThread.start();
}

QtInterface::~QtInterface(){
  disconnect(runModeWorker,SIGNAL(stoppedRunning(QString)),&window,SIGNAL(stoppedRunning(QString)));
  disconnect(runModeWorker,SIGNAL(stepped()),&window,SIGNAL(stepped()));
  disconnect(&window,SIGNAL(stopRunning()),runModeWorker,SLOT(stopRunning()));
  disconnect(this,SIGNAL(startedRunning()),&window,SIGNAL(startedRunning()));
  disconnect(this,SIGNAL(startRunning()),runModeWorker,SLOT(startRunning()));
  disconnect(this,SIGNAL(startStepping()),runModeWorker,SLOT(startStepping()));
  delete &myDeviceRegistry;
  delete &myLoader;
  delete &myCPU;
  delete &myBreakpointList;
  delete runModeWorker;
  runModeThread.quit();
  runModeThread.wait();
}

void QtInterface::Log(std::string str) {
  myOutputStream << str;
  outputChanged();
}

void QtInterface::LogLn(std::string str) {
  myOutputStream << str << std::endl;
  outputChanged();
}

void QtInterface::Trace(std::string str) {
  window.Trace(str);
}

void QtInterface::TraceLn(std::string str) {
  Trace(str + "\n");
}

RunModeWorker::RunModeWorker(QtInterface* interface, QObject * parent) :
  QObject(parent),
  interface(interface) { }

void RunModeWorker::run() {
  QString str;
  for (size_t steps = 0;running; ++steps) {
    std::string traceRecord;
    const std::string &message = interface->myCPU.ExecuteInstruction(traceRecord, false);
    if (!message.empty()) {
      if (message[0] == '.') {
        interface->Log(message.substr(1));
      } else {
        str.append("Execution stopped: " + QString::fromStdString(message));
        //interface->LogLn("Execution stopped: ");
        //interface->window.setStatusbarTemp("Execution stopped: " + message);
        break;
      }
    //} else if (myBreakpointList.Check(myCPU.ValueOfProgramCounter())) {
    } else if (interface->window.breakpoints->contains(interface->myCPU.ValueOfProgramCounter())) {
      str.append("Execution stopped at a breakpoint!");
      //interface->LogLn("Execution stopped at a breakpoint!");
      //interface->window.setStatusbarTemp((std::string) "Execution stopped at a breakpoint!");
      break;
    }
    if(stepping) {
      emit stepped();
      QThread::currentThread()->msleep(interface->window.getRunStepWait());
    }
  }
  emit stepped();
  emit stoppedRunning(str);
}
void RunModeWorker::startRunning() {
  stepping = false;
  running = true;
  run();
}

void RunModeWorker::startStepping() {
  stepping = true;
  running = true;
  run();
}

void RunModeWorker::stopRunning() {
  running = false;
}

std::string QtInterface::ProgramCounterValue() {
  return IntToString(myCPU.ValueOfProgramCounter(),8);
}

std::string QtInterface::ListExecutionTraceRecord() {
  return myCPU.ExecutionTraceRecord();
}

std::string QtInterface::ListDefaultExecutionTraceEntries() {
  return myCPU.DefaultExecutionTraceEntries();
}

void QtInterface::ClearStatistics() { myCPU.ClearStatistics(); }

void QtInterface::ListStatistics() {
  StatisticalInformationList list(myCPU);
  for (size_t t = 0; t < list.NumberOfElements(); ++t) {
    StatisticInformation info;
    list.Element(t, info);
    //myOutputStream << info.Statistic() << std::endl;
    //outputChanged();
  }
  //TODO: Return statistics in struct
}

void QtInterface::GetRegisters(QList<RegisterModel> &reglist) {
  RegisterInformationList list(myCPU);
  size_t width = 0;
  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    QString name = QString::fromStdString(info.Name());
    QString value = QString::fromStdString(info.HexValue());
    RegisterModel* reg = new RegisterModel(name, value);
    reglist.push_back(*reg);
  }
}

/*RegisterModelList QtInterface::ListRegisters(RegisterModelList* reglist) {
  RegisterInformationList list(myCPU);
  size_t width = 0;
  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    QString name = QString::fromStdString(info.Name());
    QString value = QString::fromStdString(info.HexValue());
    RegisterModel* reg = new RegisterModel(name, value);
    reglist->addRegister(*reg);
  }
  return reglist;
}*/

std::string QtInterface::ListRegisterDescription(std::string name) {
  RegisterInformationList list(myCPU);

  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    if (name == info.Name()) {
      return info.Description();
    }
  }
  LogLn("ERROR: Invalid register name!");
  window.setStatusbarTemp((std::string) "ERROR: Invalid register name!");
  return "";
}

bool QtInterface::SetRegister(std::string name, std::string value) {
  RegisterInformationList list(myCPU);

  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;

    list.Element(k, info);
    if (name == info.Name()) {
      myCPU.SetRegister(name, value);
      emit registerChanged(name, value);
      return true;
    }
  }
  window.setStatusbarTemp((std::string) "ERROR: Invalid register name!");
  return false;
}

std::string QtInterface::ListRegisterValue(std::string name) {
  RegisterInformationList list(myCPU);

  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;

    list.Element(k, info);
    if (name == info.Name()) {
      LogLn(info.HexValue());
      return info.HexValue();
    }
  }
  window.setStatusbarTemp((std::string) "ERROR: Invalid register name!");
  return "";
}

bool QtInterface::DetachDevice(size_t addressSpace, size_t deviceIndex) {
  if (myCPU.NumberOfAddressSpaces() <= addressSpace) {
    LogLn("ERROR: Invalid address space!");
    window.setStatusbarTemp((std::string)  "ERROR: Invalid address space!");
    return false;
  }
  if (!myCPU.addressSpace(addressSpace).DetachDevice(deviceIndex)) {
    LogLn("ERROR: Couldn't detach device!");
    window.setStatusbarTemp((std::string) "ERROR: Couldn't detach device!");
    return false;
  }
  return true;
}

bool QtInterface::AttachDevice(std::string name, std::string deviceArgs, size_t addressSpace) {
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address space!");
    return false;
  }

  BasicDevice *device = NULL;
  if (name == "M68681") {
    device = new M68681(deviceArgs, myCPU, &window);
    if (device == NULL || !device->ErrorMessage().empty()) {
      delete device;
      LogLn("ERROR: Couldn't create the device!");
      window.setStatusbarTemp((std::string) "ERROR: Couldn't create the device!");
      return false;
    }
  } else if (!myDeviceRegistry.Create(name, deviceArgs, myCPU, device)) {
    LogLn("ERROR: Couldn't create the device!");
    window.setStatusbarTemp((std::string) "ERROR: Couldn't create the device!");
    return false;
  }
  myCPU.addressSpace(addressSpace).AttachDevice(device);
  LogLn("Device created!");
  window.setStatusbarTemp((std::string) "Device created!");
  return true;
}

bool QtInterface::NewBreakpoint(Address address) {
  //Address address = StringToInt(addressStr);
  bool ret = window.breakpoints->addBreakpoint(address);
  return ret;
}

bool QtInterface::RemoveBreakpoint(Address address) {
  //Address address = StringToInt(addressStr);
  if (!window.breakpoints->removeBreakpoint(address)) {
    //LogLn("ERROR: Couldn't delete breakpoint!");
    window.setStatusbarTemp((std::string) "ERROR: Couldn't delete breakpoint!");
    return false;
  }
  return true;
}

bool QtInterface::FillMemoryBlock(unsigned int addressSpace, std::string addressStr, unsigned long length, std::string value) {
  Address address = StringToInt(addressStr);

  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address space!");
    return false;
  }

  if ((address+length) > myCPU.addressSpace(addressSpace).MaximumAddress()) {
    LogLn("ERROR: Invalid address!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address!");
    return false;
  }

  for (size_t i = 0; i < length; ++i) {
    Address addr = (address + i) * myCPU.Granularity();
    for (size_t t = 0; t < myCPU.Granularity(); ++t) {
      myCPU.addressSpace(addressSpace)
          .Poke(addr + t, StringToInt(std::string(value, t * 2, 2)));
    }
  }
  return true;
}

void QtInterface::ListBreakpoints() {
  for (size_t t = 0; t < myBreakpointList.NumberOfBreakpoints(); ++t) {
    Address address = 0;
    myBreakpointList.GetBreakpoint(t, address);
    //myOutputStream << IntToString(address, 8) << std::endl;
    //TODO: Return some struct with all breakpoints
  }
}

void QtInterface::ListAttachedDevices(unsigned int addressSpace) {
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    Log("ERROR: Invalid address space!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address space!");
    return;
  }

  size_t n = myCPU.addressSpace(addressSpace).NumberOfAttachedDevices();
  for (size_t k = 0; k < n; ++k) {
    AddressSpace::DeviceInformation info;
    myCPU.addressSpace(addressSpace).GetDeviceInformation(k, info);
    //myOutputStream << info.name << " {" << info.arguments << "}" << std::endl;
    //TODO: Return some struct with the info
  }
}

void QtInterface::ListDevices() {
  for (size_t t = 0; t < myDeviceRegistry.NumberOfDevices(); ++t) {
    DeviceInformation info;
    myDeviceRegistry.Information(t, info);
    //myOutputStream << info.name << std::endl;
    //TODO: Display list somewhere
  }
}

std::string QtInterface::ListDeviceScript(std::string name) {
  for (size_t t = 0; t < myDeviceRegistry.NumberOfDevices(); ++t) {
    DeviceInformation info;
    myDeviceRegistry.Information(t, info);
    if (name == info.name) {
      return info.script;
    }
  }
  LogLn("ERROR: Invalid device name!");
  window.setStatusbarTemp((std::string) "ERROR: Invalid device name!");
  return "";
}

std::string QtInterface::ListMemory(unsigned int addressSpace, std::string addressStr, unsigned int length, unsigned int wordsPerLine) {
  std::string line;
  Address address = StringToInt(addressStr);
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address space!");
    return "";
  }
  size_t numberOfWords = 0;
  for (size_t t = 0; t < length; ++t) {
    for (size_t s = 0; s < myCPU.Granularity(); ++s) {
      Byte value;
      if (myCPU.addressSpace(addressSpace)
              .Peek((address + t) * myCPU.Granularity() + s, value)) {
        line += IntToString(value, 2);
      } else {
        line += "xx";
      }
    }
    ++numberOfWords;
    if (numberOfWords >= wordsPerLine) {
      //myOutputStream << line << std::endl;
      numberOfWords = 0;
      line += "\n";
    } else {
      line += " ";
    }
  }
  return line;
  //if (!line.empty())
    //myOutputStream << line << std::endl;
}

void QtInterface::Step(int numberOfSteps) {
  if(numberOfSteps<0)
    return;
  for (int t = 0; t < numberOfSteps; ++t) {
    std::string traceRecord;
    const std::string &message = myCPU.ExecuteInstruction(traceRecord, true);
    if (!message.empty()) {
      //myOutputStream << "{SimulatorMessage {" << message << "}}" << std::endl;
      //outputChanged();
      window.setStatusbarTemp((std::string) message);
      break;
    }
    //myOutputStream << traceRecord << std::endl;
    TraceLn(traceRecord);
  }
  emit stepped();
}

void QtInterface::Reset() {
  myCPU.Reset();
  window.setStatusbarTemp((std::string) "CPU Reset!");
}

void QtInterface::Run() {
  if(window.running)
    return;
  emit startedRunning();
  emit startRunning();
  /*for (size_t steps = 0;; ++steps) {
    std::string traceRecord;
    const std::string &message = myCPU.ExecuteInstruction(traceRecord, false);
    if (!message.empty()) {
      if (message[0] == '.') {
        myOutputStream << message.substr(1) << std::flush;
        outputChanged();
      } else {
        myOutputStream << "Execution stopped: " << message << std::endl;
        outputChanged();
        window.setStatusbarTemp("Execution stopped: " + message);
        break;
      }
    //} else if (myBreakpointList.Check(myCPU.ValueOfProgramCounter())) {
    } else if (window.breakpoints->contains(myCPU.ValueOfProgramCounter())) {
      LogLn("Execution stopped at a breakpoint!");
      window.setStatusbarTemp((std::string) "Execution stopped at a breakpoint!");
      break;
    }
  }
  emit stoppedRunning();
  emit stepped();*/
}

void QtInterface::RunStepping() {
  if(window.running)
    return;
  emit startedRunning();
  emit startStepping();
  /*for (size_t steps = 0;; ++steps) {
    std::string traceRecord;
    const std::string &message = myCPU.ExecuteInstruction(traceRecord, false);
    if (!message.empty()) {
      if (message[0] == '.') {
        myOutputStream << message.substr(1) << std::flush;
        outputChanged();
      } else {
        myOutputStream << "Execution stopped: " << message << std::endl;
        outputChanged();
        window.setStatusbarTemp("Execution stopped: " + message);
        break;
      }
    //} else if (myBreakpointList.Check(myCPU.ValueOfProgramCounter())) {
    } else if (window.breakpoints->contains(myCPU.ValueOfProgramCounter())) {
      myOutputStream << "Execution stopped at a breakpoint!" << std::endl;
      outputChanged();
      window.setStatusbarTemp((std::string) "Execution stopped at a breakpoint!");
      break;
    }
    emit stepped();
  }
  emit stoppedRunning();
  emit stepped();*/
}

Address QtInterface::GetMaximumAddress(size_t addressSpace) {
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address space!");
    return 0;
  } else
    return myCPU.addressSpace(addressSpace).MaximumAddress();
}

size_t QtInterface::ListNumberOfAddressSpaces() {
  return myCPU.NumberOfAddressSpaces();
}

const unsigned int QtInterface::ListGranularity() {
  return myCPU.Granularity();
}

void QtInterface::SetMemory(size_t addressSpace, std::string address, std::string value) {
  Address addressInt = StringToInt(address);

  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address space!");
    return;
  }
  if (addressInt > myCPU.addressSpace(addressSpace).MaximumAddress()) {
    LogLn("ERROR: Invalid address!");
    window.setStatusbarTemp((std::string) "ERROR: Invalid address!");
    return;
  }
  addressInt *= myCPU.Granularity();
  for (size_t t = 0; t < myCPU.Granularity(); ++t) {
    myCPU.addressSpace(addressSpace)
        .Poke(addressInt + t, StringToInt(std::string(value, t * 2, 2)));
  }
}

bool QtInterface::LoadProgram(const std::string &filename, int addressSpace) {
  std::string result = myLoader.Load(filename, addressSpace);
  if(result.empty()) {
    window.setStatusbarTemp(QString("Program loaded!"));
    emit programLoaded(filename);
    return true;
  }
  window.setStatusbarTemp("Error: " + result);
  LogLn(result);
  return false;
}

/*
 *
 * Old BSVC interface command handling
 *
 *
*/

void QtInterface::ExecuteCommand(const std::string &command) {
  for (size_t t = 0; t < myNumberOfCommands; ++t) {
    if (command.find(ourCommandTable[t].name) == 0) {
      int len = ourCommandTable[t].name.length();
      if(command.length() <= len) {
        LogLn("ERROR: Invalid arguments!");
        break;
      }
      std::string args = command.substr(len + 1);
      (this->*ourCommandTable[t].mfp)(args);
      return;
    }
  }
  LogLn("ERROR: Unknown command!");
}

QtInterface::CommandTable QtInterface::ourCommandTable[] = {
    {"AddBreakpoint", &QtInterface::AddBreakpoint},
    {"AttachDevice", &QtInterface::AttachDevice},
    {"ClearStatistics", &QtInterface::ClearStatistics},
    {"DetachDevice", &QtInterface::DetachDevice},
    {"DeleteBreakpoint", &QtInterface::DeleteBreakpoint},
    {"FillMemoryBlock", &QtInterface::FillMemoryBlock},
    {"ListAttachedDevices", &QtInterface::ListAttachedDevices},
    {"ListBreakpoints", &QtInterface::ListBreakpoints},
    {"ListDevices", &QtInterface::ListDevices},
    {"ListDeviceScript", &QtInterface::ListDeviceScript},
    {"ListExecutionTraceRecord", &QtInterface::ListExecutionTraceRecord},
    {"ListDefaultExecutionTraceEntries",
     &QtInterface::ListDefaultExecutionTraceEntries},
    {"ListGranularity", &QtInterface::ListGranularity},
    {"ListMemory", &QtInterface::ListMemory},
    {"ListMaximumAddress", &QtInterface::ListMaximumAddress},
    {"ListNumberOfAddressSpaces", &QtInterface::ListNumberOfAddressSpaces},
    {"ListRegisters", &QtInterface::ListRegisters},
    {"ListRegisterValue", &QtInterface::ListRegisterValue},
    {"ListRegisterDescription", &QtInterface::ListRegisterDescription},
    {"ListStatistics", &QtInterface::ListStatistics},
    {"LoadProgram", &QtInterface::LoadProgram},
    {"ProgramCounterValue", &QtInterface::ProgramCounterValue},
    {"Reset", &QtInterface::Reset},
    {"Run", &QtInterface::Run},
    {"SetMemory", &QtInterface::SetMemory},
    {"SetRegister", &QtInterface::SetRegister},
    {"Step", &QtInterface::Step}};

void QtInterface::ProgramCounterValue(const std::string &) {
  myOutputStream << std::hex << myCPU.ValueOfProgramCounter() << std::endl;
  outputChanged();
}

void QtInterface::ListExecutionTraceRecord(const std::string &) {
  LogLn(myCPU.ExecutionTraceRecord());
  outputChanged();
}

void QtInterface::ListDefaultExecutionTraceEntries(const std::string &) {
  LogLn(myCPU.DefaultExecutionTraceEntries());
  outputChanged();
}

void QtInterface::ClearStatistics(const std::string &) { ClearStatistics(); }

void QtInterface::ListStatistics(const std::string &) {
  StatisticalInformationList list(myCPU);
  for (size_t t = 0; t < list.NumberOfElements(); ++t) {
    StatisticInformation info;
    list.Element(t, info);
    myOutputStream << info.Statistic() << std::endl;
  }
  outputChanged();
}

void QtInterface::ListRegisters(const std::string &) {
  RegisterInformationList list(myCPU);
  size_t width = 0;
  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    width = width > info.Name().length()?width:info.Name().length();
  }
  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    myOutputStream << std::setw(width) << std::left << std::setfill(' ')
                   << info.Name() << " = " << info.HexValue() << std::endl;
  }
  outputChanged();
}

// Lists a register's description.
void QtInterface::ListRegisterDescription(const std::string &args) {
  RegisterInformationList list(myCPU);
  std::istringstream in(args);
  std::string name;
  in >> name;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    outputChanged();
    return;
  }
  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    if (name == info.Name()) {
      LogLn(info.Description());
      return;
    }
  }
  LogLn("ERROR: Invalid register name!");
}

void QtInterface::SetRegister(const std::string &args) {
  std::istringstream in(args);
  RegisterInformationList list(myCPU);
  std::string name;
  std::string value;
  in >> name >> value;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  /*for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    if (name == info.Name()) {
      myCPU.SetRegister(name, value);
      return;
    }
  }*/
  if(!SetRegister(name, value))
    LogLn("ERROR: Invalid register name!");
}

// Lists the value of one of the cpu's registers.
void QtInterface::ListRegisterValue(const std::string &args) {
  std::istringstream in(args);
  RegisterInformationList list(myCPU);
  std::string name;
  in >> name;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  for (size_t k = 0; k < list.NumberOfElements(); ++k) {
    RegisterInformation info;
    list.Element(k, info);
    if (name == info.Name()) {
      LogLn(info.HexValue());
      return;
    }
  }
  LogLn("ERROR: Invalid register name!");
}

void QtInterface::DetachDevice(const std::string &args) {
  std::istringstream in(args);
  size_t addressSpace;
  size_t deviceIndex;
  in >> addressSpace >> deviceIndex;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (myCPU.NumberOfAddressSpaces() <= addressSpace) {
    LogLn("ERROR: Invalid address space!");
    return;
  }
  if (!myCPU.addressSpace(addressSpace).DetachDevice(deviceIndex)) {
    LogLn("ERROR: Couldn't detach device!");
  }
}

// Attaches a device to the simulator.
void QtInterface::AttachDevice(const std::string &args) {
  std::istringstream in(args);
  std::string name;
  std::string deviceArgs;
  size_t addressSpace;
  char c;
  in >> addressSpace >> name >> c;
  in.unsetf(std::ios::skipws);
  if (c != '{') {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  std::getline(in, deviceArgs, '}');
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    return;
  }
  BasicDevice *device;
  //if (!myDeviceRegistry.Create(name, deviceArgs, myCPU, device)) {
  if (!AttachDevice(name, deviceArgs, addressSpace)) {
    LogLn("ERROR: Couldn't create the device!");
    return;
  }
  //myCPU.addressSpace(addressSpace).AttachDevice(device);
}

void QtInterface::AddBreakpoint(const std::string &args) {
  std::istringstream in(args);
  Address address;
  in >> std::hex >> address;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  NewBreakpoint(address);
}

void QtInterface::DeleteBreakpoint(const std::string &args) {
  std::istringstream in(args);
  Address address;
  in >> std::hex >> address;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (!RemoveBreakpoint(address)) {
    LogLn("ERROR: Couldn't delete breakpoint!");
  }
}

void QtInterface::FillMemoryBlock(const std::string &args) {
  std::istringstream in(args);
  unsigned int addressSpace;
  Address address;
  unsigned long length;
  std::string value;

  in >> addressSpace >> std::hex >> address >> std::hex >> length >> value;

  // Make sure we were able to read the arguments
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    return;
  }

  if (address > myCPU.addressSpace(addressSpace).MaximumAddress()) {
    LogLn("ERROR: Invalid address!");
    return;
  }

  for (size_t i = 0; i < length; ++i) {
    Address addr = (address + i) * myCPU.Granularity();
    for (size_t t = 0; t < myCPU.Granularity(); ++t) {
      myCPU.addressSpace(addressSpace)
          .Poke(addr + t, StringToInt(std::string(value, t * 2, 2)));
    }
  }
}

void QtInterface::ListBreakpoints(const std::string &) {
  for (size_t t = 0; t < myBreakpointList.NumberOfBreakpoints(); ++t) {
    Address address = 0;
    myBreakpointList.GetBreakpoint(t, address);
    LogLn(IntToString(address, 8));
  }
}

void QtInterface::ListAttachedDevices(const std::string &args) {
  std::istringstream in(args);
  unsigned int addressSpace;
  in >> addressSpace;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    return;
  }

  size_t n = myCPU.addressSpace(addressSpace).NumberOfAttachedDevices();
  for (size_t k = 0; k < n; ++k) {
    AddressSpace::DeviceInformation info;
    myCPU.addressSpace(addressSpace).GetDeviceInformation(k, info);
    myOutputStream << info.name << " {" << info.arguments << "}" << std::endl;
  }
  outputChanged();
}

void QtInterface::ListDevices(const std::string &) {
  for (size_t t = 0; t < myDeviceRegistry.NumberOfDevices(); ++t) {
    DeviceInformation info;
    myDeviceRegistry.Information(t, info);
    myOutputStream << info.name << std::endl;
  }
  outputChanged();
}

void QtInterface::ListDeviceScript(const std::string &args) {
  std::istringstream in(args);
  std::string name;
  in >> name;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  for (size_t t = 0; t < myDeviceRegistry.NumberOfDevices(); ++t) {
    DeviceInformation info;
    myDeviceRegistry.Information(t, info);
    if (name == info.name) {
      LogLn(info.script);
      return;
    }
  }
  LogLn("ERROR: Invalid device name!");
}

void QtInterface::ListMemory(const std::string &args) {
  std::istringstream in(args);
  unsigned int addressSpace;
  Address address;
  unsigned int length;
  unsigned int wordsPerLine;
  std::string line;
  in >> addressSpace >> std::hex >> address >> length >> wordsPerLine;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    return;
  }
  size_t numberOfWords = 0;
  for (size_t t = 0; t < length; ++t) {
    for (size_t s = 0; s < myCPU.Granularity(); ++s) {
      Byte value;
      if (myCPU.addressSpace(addressSpace)
              .Peek((address + t) * myCPU.Granularity() + s, value)) {
        line += IntToString(value, 2);
      } else {
        line += "xx";
      }
    }
    ++numberOfWords;
    if (numberOfWords >= wordsPerLine) {
      myOutputStream << line << std::endl;
      numberOfWords = 0;
      line = "";
    } else {
      line += " ";
    }
  }
  if (!line.empty())
    myOutputStream << line << std::endl;
  outputChanged();
}

void QtInterface::Step(const std::string &args) {
  std::istringstream in(args);
  int numberOfSteps;
  in >> numberOfSteps;
  if (!in) {
    myOutputStream << "ERROR: Invalid arguments!" << std::endl;
    return;
  }
  Step(numberOfSteps);
}

void QtInterface::Reset(const std::string &) {
  myCPU.Reset();
}

void QtInterface::Run(const std::string &args) {
  /*std::istringstream in(args);
  std::string name;
  char c;
  in >> c;
  in.unsetf(std::ios::skipws);
  if (c != '{') {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  std::getline(in, name, '}');
  for (size_t steps = 0;; ++steps) {
    std::string traceRecord;
    const std::string &message = myCPU.ExecuteInstruction(traceRecord, false);
    if (!message.empty()) {
      if (message[0] == '.') {
        myOutputStream << message.substr(1) << std::flush;
      } else {
        myOutputStream << "Execution stopped: " << message << std::endl;
        break;
      }
    } else if (myBreakpointList.Check(myCPU.ValueOfProgramCounter())) {
      myOutputStream << "Execution stopped at a breakpoint!" << std::endl;
      break;
    }
    else if ((steps & 0x03FF) == 0) {
#ifdef _WIN32
      static HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
      DWORD r;
      PeekNamedPipe(h, NULL, 0, NULL, &r, NULL);
      if(r>0) {
#else
      fd_set rfds;
      struct timeval tv;
      int retval;

      // We're going to poll STDIN to see if any input is available
      FD_ZERO(&rfds);
      FD_SET(0, &rfds);

      // Don't wait at all
      tv.tv_sec = 0;
      tv.tv_usec = 0;

      retval = select(1, &rfds, NULL, NULL, &tv);

      // Stop running if data is ready to be read
      if (retval == 1) {
        // Read the "StopRunning" Command

#endif
        std::string dummy;
        std::getline(myInputStream, dummy);

        myOutputStream << "Execution Interrupted!" << std::endl;
        break;
      }
    }
  }*/
  return Run();
}

void QtInterface::ListMaximumAddress(const std::string &args) {
  std::istringstream in(args);
  size_t addressSpace;
  in >> addressSpace;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    return;
  }
  myOutputStream << std::hex << myCPU.addressSpace(addressSpace).MaximumAddress()
                 << std::endl;
  outputChanged();
}

void QtInterface::ListNumberOfAddressSpaces(const std::string &) {
  myOutputStream << std::dec << myCPU.NumberOfAddressSpaces() << std::endl;
  outputChanged();
}

void QtInterface::ListGranularity(const std::string &) {
  myOutputStream << std::dec << myCPU.Granularity() << std::endl;
  outputChanged();
}

void QtInterface::SetMemory(const std::string &args) {
  std::istringstream in(args);
  std::string value;
  size_t addressSpace;
  Address address;
  in >> addressSpace >> std::hex >> address >> value;
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  if (addressSpace >= myCPU.NumberOfAddressSpaces()) {
    LogLn("ERROR: Invalid address space!");
    return;
  }
  if (address > myCPU.addressSpace(addressSpace).MaximumAddress()) {
    LogLn("ERROR: Invalid address!");
    return;
  }
  address *= myCPU.Granularity();
  for (size_t t = 0; t < myCPU.Granularity(); ++t) {
    myCPU.addressSpace(addressSpace)
        .Poke(address + t, StringToInt(std::string(value, t * 2, 2)));
  }
}

void QtInterface::LoadProgram(const std::string &args) {
  std::istringstream in(args);
  size_t addressSpace;
  std::string name;
  char c;
  in >> addressSpace >> c;
  in.unsetf(std::ios::skipws);
  if (c != '{') {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  std::getline(in, name, '}');
  if (!in) {
    LogLn("ERROR: Invalid arguments!");
    return;
  }
  std::string ret = myLoader.Load(name, addressSpace);
  if(ret.empty())
    emit programLoaded(name);
  else
    LogLn(ret);
}

void QtInterface::outputChanged() {
  this->window.outputChanged();
}
