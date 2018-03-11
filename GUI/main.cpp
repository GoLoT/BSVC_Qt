#include "mainwindow.h"
#include <QApplication>

//68k Includes
//#include <memory>
//#include "Framework/Interface.hpp"
#include "qtinterface.h"
/*#include "M68k/sim68000/m68000.hpp"
#include "M68k/sim68360/cpu32.hpp"
#include "M68k/devices/DeviceRegistry.hpp"
#include "M68k/loader/Loader.hpp"*/

QtInterface* interfacePtr = NULL;

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  return a.exec();
}
