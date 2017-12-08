/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CPUTPARSER_H
#define CPUTPARSER_H

class CommandParser {
public:
	~CommandParser();
	void ParseConfigurationOptions(cString arguments, cString delimiter = _L("-"));
	void CleanConfigurationOptions(void);

	bool GetParameter(cString arg);
	bool GetParameter(cString arg, double *pOut);
	bool GetParameter(cString arg, int *pOut);
	bool GetParameter(cString arg, unsigned int *pOut);
	bool GetParameter(cString arg, cString *pOut);
	bool GetParameter(cString arg, char *pOut);

private:
	std::map<cString, cString> m_ArgumentMap;
};

#endif // CPUTPARSER_H
