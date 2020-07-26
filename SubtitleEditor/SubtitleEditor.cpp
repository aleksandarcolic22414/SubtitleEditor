// File: SubtitleEditor.cpp
//
// This one is used to manage subtitles.
//

// Every subtitle has the same format. It has N units and every unit is formated like:
// 1. Ordinal 
// 2. Start time --> End time (time format: hh:mm:ss,mss)
// 3. N rows with subtitle 
//
// It's pretty easy to parse these, so let's dig in.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <windows.h>

using namespace std;

#define STRLITSIZE(strlit) ((ARRAYSIZE(strlit)) - 1)

// Convert time string with format hh:mm:ss,mss to miliseconds.
// Returns 0 if conversion was successful, -1 otherwise.
//
__forceinline
int TimeToMs(
	const char *time,
	long long &outMs)
{
	int err = 0;
	if (time == nullptr)
	{
		err = -1;
	}

	char *extracter = (char *)time;

	// Try to parse time
	//
	long long hh, mm, ss, ms;
	hh = mm = ss = ms = 0;

	// Extract hours.
	//
	if (!err)
	{
		for (int i = 0; i < STRLITSIZE("hh"); ++i)
		{
			if (!isdigit(*extracter))
			{
				err = -1;
				break;
			}

			hh = hh * 10 + *extracter++ - '0';
		}

		if (*extracter != ':')
		{
			err = -1;
		}

		++extracter;
	}

	// Extract minutes.
	//
	if (!err)
	{
		for (int i = 0; i < STRLITSIZE("mm"); ++i)
		{
			if (!isdigit(*extracter))
			{
				err = -1;
				break;
			}

			mm = mm * 10 + *extracter++ - '0';
		}

		if (*extracter != ':')
		{
			err = -1;
		}

		++extracter;
	}

	// Extract seconds.
	//
	if (!err)
	{
		for (int i = 0; i < STRLITSIZE("ss"); ++i)
		{
			if (!isdigit(*extracter))
			{
				err = -1;
				break;
			}

			ss = ss * 10 + *extracter++ - '0';
		}

		if (*extracter != ',')
		{
			err = -1;
		}

		++extracter;
	}

	// Extract miliseconds.
	//
	if (!err)
	{
		for (int i = 0; i < STRLITSIZE("mss"); ++i)
		{
			if (!isdigit(*extracter))
			{
				err = -1;
				break;
			}

			ms = ms * 10 + *extracter++ - '0';
		}

		if (*extracter != '\0' && *extracter != '\n')
		{
			err = -1;
		}
	}

	if (err)
	{
		cerr << "Error occured while converting time to miliseconds." << endl;
		cerr << "Expected time format is: hh:mm:ss,mss" << endl;
	}

	outMs = (long long)ms + 1000 * ss + 1000 * 60 * mm + 1000 * 60 * 60 * hh;
	return err;
}

// Convert milisecons to time format hh:mm:ss,mss
//
int MsToTime(
	long long ms,
	char *timeBuff)
{
	int err = 0;
	if (timeBuff == nullptr || ms < 0)
	{
		// timeBuff = nullptr;
		err = -1;
	}

	if (!err)
	{
		// Take hours, minutes and seconds from miliseconds.
		//
		long long hh, mm, ss;
		hh = ms / (1000*60*60);
		ms %= 1000*60*60;

		mm = ms / (1000*60);
		ms %= 1000*60;

		ss = ms / 1000;
		ms %= 1000;

		*timeBuff++ = hh / 10 + '0';
		*timeBuff++ = hh % 10 + '0';
		*timeBuff++ = ':';

		*timeBuff++ = mm / 10 + '0';
		*timeBuff++ = mm % 10 + '0';
		*timeBuff++ = ':';

		*timeBuff++ = ss / 10 + '0';
		*timeBuff++ = ss % 10 + '0';
		*timeBuff++ = ',';

		*timeBuff++ = ms / 100 + '0';
		*timeBuff++ = ms % 100 / 10 + '0';
		*timeBuff++ = ms % 100 % 10 + '0';

		*timeBuff = '\0';
	}
	
	if (err)
	{
		cerr << "Error occured while converting: " + to_string(ms) + " to time." << endl;
		cerr << "Time buffer can not be null and time must be positive value." << endl;
	}

	return err;
}

// Extracts start and end time from given line.
// Line shoud be fomated like:
// hh:mm:ss,mss --> hh:mm:ss,mss
//
__forceinline
int parseStartAndEndTime(
	const char *input,
	char *startTimeBuff,
	char *endTimeBuff)
{
	if (input == nullptr || startTimeBuff == nullptr || endTimeBuff == nullptr)
	{
		cerr << "Null parameter(s) passed in parseStartAndEndTime." << endl;
		cerr << "Function parseStartAndEndTime aborted." << endl;
		return -1;
	}

	int err = 0;
	char *extracter = (char *)input;

	// Extract start time.
	//
	while (*extracter && *extracter != ' ' && extracter - input < STRLITSIZE("hh:mm:ss,mss"))
	{
		*startTimeBuff++ = *extracter++;
	}

	*startTimeBuff = '\0';
	if (*extracter == '\0')
	{
		err = -1;
	}

	if (!err)
	{
		for (int i = 0; i < STRLITSIZE(" --> "); ++i)
		{
			++extracter;
			if (*extracter == '\0')
			{
				err = -1;
			}
		}
	}

	if (!isdigit(*extracter))
	{
		err = -1;
	}

	// Extract end time.
	//
	if (!err)
	{
		while (*extracter && *extracter != '\n' &&
			   extracter - (input + STRLITSIZE("hh:mm:ss,mss --> ")) < STRLITSIZE("hh:mm:ss,mss"))
		{
			*endTimeBuff++ = *extracter++;
		}
	}

	*endTimeBuff = '\0';
	if (err)
	{
		cerr << "Error occured in function: parseStartAndEndTime" << endl;
		cerr << "Expected format is: hh:mm:ss,mss --> hh:mm:ss,mss" << endl;
	}

	return err;
}

// Check if ord is constucted out of digits.
//
int CheckOrdinal(
	string &ord)
{
	for (int i = 0; ord[i]; ++i)
	{
		if (!isdigit(ord[i]))
		{
			cerr << "Wrong ordinal number: " << ord << endl;
			return -1;
		}
	}

	return 0;
}

// This one will create new file in format pathToSub_fixed.srt with fixed subtitle.
//
// Params:
//	__in pathToSub - Path to input subtitle file.
//	__in incBySec - Move subtitle to left (-) or right (+) by incBySec seconds.
//	__in proportion - Proportion of movie duration and subtitle duration. It is used to
//	stretch or shrink subtitle times in orderd to synchronize movie and subtitle.
//	__in delOutFileOnErr - Delete output file if error occurs.
//
int FixSubtitle(
	string pathToSub,
	long long incBySec = 0,
	long double proportion = 1,
	bool delOutFileOnErr = false)
{
	int err = 0;

	// Open in and out files.
	//
	ifstream infile(pathToSub);
	if (!infile.good())
	{
		cerr << "Problem occured while opening input file: " + pathToSub << endl;
		return -1;
	}

	string outFilePath = pathToSub.substr(0, pathToSub.size() - 4) + "_fixed.srt";
	ofstream outfile(outFilePath);
	if (!outfile.good())
	{
		cerr << "Problem occured while opening output file: " + outFilePath << endl;
		err = -1;
	}

	// Read lines in format:
	// 1. Ordinal 
	// 2. Start time --> End time (time format: hh:mm:ss,ms)
	// 3. N rows with subtitle 
	//
	// Print output to file in parallel.
	//

	// Use char arrays to minimize conversion time.
	//
	string inData;
	char startTime[256], endTime[256];
	int lineNumber = 0;
	while (!err)
	{
		// Scan and print ordinal.
		//
		getline(infile, inData);
		++lineNumber;

		// If ordinal is empty we've reached EOF.
		//
		if (inData == "" || infile.eof())
		{
			break;
		}

		// Check ordinal
		//
		if (CheckOrdinal(inData) == -1)
		{
			err = -1;
			break;
		}

		outfile << inData << endl;
		
		// Scan and print start and end time.
		//
		getline(infile, inData);
		++lineNumber;

		if (parseStartAndEndTime(inData.c_str(), startTime, endTime))
		{
			err = -1;
			break;
		}

		long long msStartTime;
		if (TimeToMs(startTime, msStartTime) == -1)
		{
			err = -1;
			break;
		}

		long long msEndTime;
		if (TimeToMs(endTime, msEndTime) == -1)
		{
			err = -1;
			break;
		}

		// Fix subtitile times.
		//
		{
			if (MsToTime(msStartTime * proportion + 1000 * incBySec, startTime) ||
				MsToTime(msEndTime * proportion + 1000 * incBySec, endTime))
			{
				err = -1;
				break;
			}
		}

		outfile << startTime;
		outfile << " --> ";
		outfile << endTime;
		outfile << endl;

		// Print subtitle.
		//
		while (getline(infile, inData))
		{
			++lineNumber;
			if (inData == "")
			{
				outfile << "\n";
				break;
			}

			outfile << inData << endl;
		}
	}

	// Close files.
	//
	if (infile.is_open())
	{
		infile.close();
	}

	if (outfile.is_open())
	{
		outfile.close();
	}
	
	if (err)
	{
		cerr << "Error on line " + to_string(lineNumber) << endl;
		cerr << "Program terminated abnormally." << endl;

		if (delOutFileOnErr)
		{
			cerr << "Deleting output file: " << outFilePath << endl;
			if (remove(outFilePath.c_str()))
			{
				cerr << "Unable to delete file: " << outFilePath << endl;
			}
		}
	}

	return err;
}

int CopyFile(
	string pathToSub)
{
	ifstream infile(pathToSub);
	if (!infile.good())
	{
		cerr << "Problem occured while opening input file: " + pathToSub;
		return -1;
	}

	string outFilePath = pathToSub.substr(0, pathToSub.size() - 4) + "_copy.srt";
	ofstream outfile(outFilePath);
	if (!outfile.good())
	{
		cerr << "Problem occured while opening output file: " + outFilePath;
		return -1;
	}

	string s;
	while (infile >> s)
	{
		outfile << s << endl;
	}

	// Close files.
	//
	if (infile.is_open())
	{
		infile.close();
	}

	if (outfile.is_open())
	{
		outfile.close();
	}

	return 0;
}

// Split string on words. Word is separated from another word with ' '
// or it is between quotes where spaces are considered as part of that word.
//
vector<string> SplitArgsFromCmd(
	const string &s)
{
	vector<string> elems;
	size_t i = 0;
	string word;
	while (s[i])
	{
		word.clear();
		while (s[i] == ' ')
		{
			++i;
		}

		// Special handle arguments in quotes.
		//
		if (s[i] == '"')
		{
			++i;
			while (s[i] && s[i] != '"')
			{
				word += s[i++];
			}

			if (s[i] == '"')
			{
				++i;
			}
		}
		else
		{
			while (s[i] && s[i] != ' ')
			{
				word += s[i++];
			}
		}

		if (word.size() > 0)
		{
			elems.push_back(word);
		}
	}

	return elems;
}

// Clear console.
//
void ClearConsole()
{
#if defined _WIN32
	system("cls");
#elif defined (__LINUX__) || defined (__gnu_linux__) || defined (__linux__) || defined (__APPLE__)
	system("clear");
#endif
}

// Displays help message for mrmots.
//
void DisplayHelpMessage()
{
	ClearConsole();

	cout << "Welcome to subtitle editor." << endl << endl;
	cout << "This program will create new subtitle file in format <input_subtitle_path>_fixed.srt"
			" in same folder as input subtitle." << endl;
	cout << "To move subtitle by n seconds, specify -incby parameter." << endl;
	cout << "To synchronize subtitle with movie specify -mdur and -sdur parameters." << endl;
	cout << "Available parameters:" << endl;
	cout << "	-path		Absolute path to subtitle file." << endl;
	cout << "	-incby		Increment (+) or decrement (-) subtitle position in time by input seconds." << endl;
	cout << "	-mdur		Movie duration in format: hh:mm:ss,mss" << endl;
	cout << "	-sdur		Subtitle duration in format: hh:mm:ss,mss" << endl;
	cout << "	-help		Displays this message." << endl;
	cout << "	-exit		Exits program." << endl << endl;
	cout << "Example: -path \"C:\\Users\\Desktop\\mysubtitle.srt\" -mdur 02:10:15,000 -sdur 02:15:13,123 -incby -5" << endl;
}

// Scan user input.
//
int scanInput(
	string &filePath,
	bool &inFileSpec,
	string &movDur,
	bool &movDurSpec,
	string &subDur,
	bool &subDurSpec,
	int &incBySec,
	bool &incBySecSpec,
	size_t &exit)
{
	// Split input into words and scan one word at the time.
	//
	cout << "Command: ";
	string inCommand;
	getline(cin, inCommand);

	size_t inCnt = 0;
	vector<string> userInput = SplitArgsFromCmd(inCommand);
	if (userInput.size() == 0)
	{
		cerr << "Empty command. Try -h for help." << endl;
		return -1;
	}

	// Scan input parameters.
	//
	while (inCnt < userInput.size())
	{
		if (userInput[inCnt] == "cls")
		{
			ClearConsole();
			return -1;
		}
		else if (userInput[inCnt] == "exit" ||
			userInput[inCnt] == "q" || 
			userInput[inCnt] == "-exit" || 
			userInput[inCnt] == "-quit")
		{
			exit = 1;
			return -1;
		}
		else if (userInput[inCnt] == "-path")
		{
			if (++inCnt >= userInput.size())
			{
				cerr << "Bad file path." << endl;
				return -1;
			}

			filePath = userInput[inCnt];
			inFileSpec = true;
		}
		else if (userInput[inCnt] == "-mdur")
		{
			if (++inCnt >= userInput.size() || userInput[inCnt].size() != strlen("hh:mm:ss,mss"))
			{
				cerr << "Bad movie duration." << endl;
				cerr << "Expected format: hh:mm:ss,mss" << endl;
				return -1;
			}

			movDur = userInput[inCnt];
			movDurSpec = true;
		}
		else if (userInput[inCnt] == "-sdur")
		{
			if (++inCnt >= userInput.size() || userInput[inCnt].size() != strlen("hh:mm:ss,mss"))
			{
				cerr << "Bad subtitle duration." << endl;
				cerr << "Expected format: hh:mm:ss,mss" << endl;
				return -1;
			}

			subDur = userInput[inCnt];
			subDurSpec = true;
		}
		else if (userInput[inCnt] == "-incby")
		{
			if (++inCnt >= userInput.size() || (userInput[inCnt][0] != '-' && !isdigit(userInput[inCnt][0])) ||
				(userInput[inCnt][0] == '-' && !isdigit(userInput[inCnt][1])))
			{
				cerr << "Bad increment parameter." << endl;
				return -1;
			}

			incBySec = atoi(userInput[inCnt].c_str());
			incBySecSpec = true;
		}
		else if (userInput[inCnt] == "/help" ||
				 userInput[inCnt] == "/h" ||
				 userInput[inCnt] == "help" ||
				 userInput[inCnt] == "-help")
		{
			DisplayHelpMessage();
			return -1;
		}
		else
		{
			cerr << "Invalid parameter: " << userInput[inCnt] << endl;
			return -1;
		}

		++inCnt;
	}

	return 0;
}

int main()
{
	DisplayHelpMessage();

	size_t exit = 0;
	while (!exit)
	{
		string filePath;
		string movDur;
		string subDur;
		int incBySec = 0;

		bool inFileSpec = false;
		bool movDurSpec = false;
		bool subDurSpec = false;
		bool incBySecSpec = false;

		if (scanInput(
				filePath,
				inFileSpec,
				movDur,
				movDurSpec,
				subDur,
				subDurSpec,
				incBySec,
				incBySecSpec,
				exit))
		{
			// Problem occur while scaning user input.
			//
			continue;
		}

		long long movDurInMs;
		long long subDurInMs;
		long double proportion = 1;

		if (!inFileSpec)
		{
			cerr << "No input file specified." << endl;
			return 0;
		}

		// If we have durations, make proportion.
		//
		if (movDurSpec && subDurSpec)
		{
			if (TimeToMs(movDur.c_str(), movDurInMs))
			{
				cerr << "Bad movie duration: " << movDur << endl;
				return 0;
			}

			if (TimeToMs(subDur.c_str(), subDurInMs))
			{
				cerr << "Bad subtitle duration: " << subDur << endl;
				return 0;
			}

			if (!movDurInMs || !subDurInMs)
			{
				cerr << "Movie and subtitle duration con not be 0." << endl;
				return 0;
			}

			proportion = (long double)movDurInMs / subDurInMs;
		}

		cout << "Increment subtitle by seconds: " << incBySec << endl;
		cout << "Fixing rate for subtitle: " << proportion << endl;

		FixSubtitle(filePath, incBySec, proportion, true);
	}

	return 0;
}