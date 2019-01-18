#ifndef VERSION_H
#define VERSION_H

#include <QString>


/*-------------------------------------------------------------------
 *
 *  Version 3.0.0      Released 11th September 2017
 *
 * First release of kabtrak
 *
 *
 *  Version 3.0.1      Released 25th September 2017
 *
 * Check tool in calibration message box.
 * Can enable/disable in Cabinet options.
 *
 *  Version 3.0.2       Released 13th November 2017
 *
 * Made batch and work order number more configurable.
 * Added function key testing when cabinet not available for testing.
 * Added power status to guest screen.
 *
 *   Version 3.0.3       Released 19th December 2017
 *
 * Batch and work order labels are configurable in the trak.ini file.
 * When these parameters are changed Kabtrak has to be restarted.
 *
 * ------------------------------------------------------------------*/


namespace Version
{
QString getVersionString();

static const int major = 3;
static const int minor = 0;
static const int release = 3;

}
#endif // VERSION_H

