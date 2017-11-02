// mmf modified version, these set FileVersion and ProductVersion used by VerRes.rc
#define rmj		1
#define rmm		9		// Imago (R#)
#define rel     1040055.00       // mmf added to designate release number for a given build

#define rup		211226  // mmf make sure this has 6 digits otherwise version number conversion for esc display may fail,
						// rup is specified in octal, so for instance 11/9/2010 (US format)
						// Year: 2010 - use last two digits: 10, convert to octal = 12.
						// Month: 11 - convert to octal = 13
						// Day: 9 - convert to octal = 11
						// Actual rup value = 121311.

#define szVerName	""
#define szVerUser	"CURTC3"
