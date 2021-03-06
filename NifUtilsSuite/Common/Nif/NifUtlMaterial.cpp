/**
 *  file:   NifUtlMaterial.h
 *  class:  NifUtlMaterialList
 *
 *  data structures handling materials from nif.xml
 *
 */

//-----  INCLUDES  ------------------------------------------------------------
#include "Common\Nif\NifUtlMaterial.h"
#include "Common\Util\Configuration.h"
#include "gen/enums.h"
#include <fstream>
#include <sstream>
#include <string>

using namespace Niflib;

//-----  DEFINES  -------------------------------------------------------------
NifUtlMaterialList*	NifUtlMaterialList::_pInstance = NULL;

//-----  NifUtlMaterialList()  ------------------------------------------------
NifUtlMaterialList::NifUtlMaterialList()
{}

//-----  ~NifUtlMaterialList()  -----------------------------------------------
NifUtlMaterialList::~NifUtlMaterialList()
{
	_pInstance = NULL;
}

//-----  getInstance()  -------------------------------------------------------
NifUtlMaterialList* NifUtlMaterialList::getInstance()
{
	if (_pInstance == NULL)		initInstance("./nif.xml", "SkyrimHavokMaterial", "SKY_HAV_");

	return _pInstance;
}

//-----  initInstance()  ------------------------------------------------------
bool NifUtlMaterialList::initInstance(const string fileName, const string matScanTag, const string matScanName)
{
	if (_pInstance == NULL)
	{
		_pInstance = new NifUtlMaterialList();
		_pInstance->initializeMaterialMap (fileName, matScanTag, matScanName);
		_pInstance->initializeBodyPartList(fileName);

	}  //  if (_pInstance == NULL)

	return (_pInstance != NULL);
}

//-----  initializeMaterialMap()  ---------------------------------------------
void NifUtlMaterialList::initializeMaterialMap(const string fileName, const string matScanTag, const string matScanName)
{
	Configuration*	pConfig     (Configuration::getInstance());
	ifstream		streamIn;
	char			cbuffer     [10000] = {0};
	char			txtTag      [200]   = {0};
	char			txtName     [200]   = {0};
	bool			isMatSection(false);

	//  reset old settings
	reset();

	//  open nif.xml
	streamIn.open(fileName.c_str(), ifstream::in);

	sprintf(cbuffer, "^%cOpening '%s': %s", (streamIn.good() ? '2' : '4'), fileName.c_str(), (streamIn.good() ? "OK" : "FAILED"));
	_userMessages.push_back(cbuffer);

	sprintf(txtTag, "<enum name=\"%s\" storage=\"uint\">", matScanTag.c_str());
	sprintf(txtName, "name=\"%s", matScanName.c_str());

	//  on valid input
	while (streamIn.good())
	{
		//  read next row
		streamIn.getline(cbuffer, 10000);

		//  search start of material definition
		if (strstr(cbuffer, txtTag) != NULL)
		{
			isMatSection = true;
		}

		//  row within material definition section
		if (isMatSection)
		{
			//  valid material definition for SKYrim?
			if ((strstr(cbuffer, "<option value=\"") != NULL) &&
				(strstr(cbuffer, txtName) != NULL))
			{
				NifUtlMaterial	matNew;
				char*			pStart(strstr(cbuffer, "value="));
				char*			pEnd  (strstr(cbuffer, "\" name="));
				bool			doSkip(false);

				//  read material code
				matNew._code = (unsigned int) atof(pStart + 7);

				//  parse and read definition name
				pStart = pEnd + 8;
				pEnd   = strstr(pStart, "\">");
				*pEnd  = 0;
				matNew._defName = pStart;

				//  parse and read user readable name
				pStart = pEnd + 2;
				pEnd   = strstr(pStart, "</option>");
				*pEnd  = 0;

				//  parse skipping entries
				for (auto pIter=pConfig->_matScanIgnore.begin(), pEnd=pConfig->_matScanIgnore.end(); pIter != pEnd; ++pIter)
				{
					if (strstr(pStart, pIter->c_str()) != NULL)
					{
						doSkip = true;
						break;
					}
				}
				if (doSkip)		continue;

				//  parse prefix list
				for (auto pIter=pConfig->_matScanPrefix.begin(), pEnd=pConfig->_matScanPrefix.end(); pIter != pEnd; ++pIter)
				{
					if (strncmp(pStart, pIter->c_str(), pIter->length()) == 0)
					{
						pStart += pIter->length();	//  skip leading prefix
						break;
					}
				}

				//  skip leading spaces
				for (; *pStart == ' '; ++pStart);

				matNew._name = pStart;

				//  append material to map
				_materialMap[matNew._defName] = matNew;

				_userMessages.push_back("^3added: " + matNew._defName + " => '" + matNew._name + "'");
			}
			//  early break at end of material definition
			else if (strstr(cbuffer, "</enum>") != NULL)
			{
				break;
			}
		}  //  if (isMaterialSection)
	}  //  while (streamIn.good())

	sprintf(cbuffer, "^%cmaterials found: %d", ((_materialMap.size() > 0) ? '2' : '5'), _materialMap.size());
	_userMessages.push_back(cbuffer);

	streamIn.close();
}

//-----  initializeBodyPartList()  --------------------------------------------
void NifUtlMaterialList::initializeBodyPartList(const string fileName)
{
	ifstream		streamIn;
	char			cbuffer     [10000] = {0};
	char			txtTag      [200]   = {0};
	string			txtName;
	string			txtDefine;
	bool			isBdySection(false);

	//  reset old list
	_bodyPartMap.clear();

	//  open nif.xml
	streamIn.open(fileName.c_str(), ifstream::in);

	sprintf(cbuffer, "^%cOpening '%s': %s", (streamIn.good() ? '2' : '4'), fileName.c_str(), (streamIn.good() ? "OK" : "FAILED"));
	_userMessages.push_back(cbuffer);

	sprintf(txtTag, "<enum name=\"BSDismemberBodyPartType\" storage=\"ushort\">");

	//  on valid input
	while (streamIn.good())
	{
		//  read next row
		streamIn.getline(cbuffer, 10000);

		//  search start of material definition
		if (strstr(cbuffer, txtTag) != NULL)
		{
			isBdySection = true;
		}

		//  row within material definition section
		if (isBdySection)
		{
			//  valid material definition for SKYrim?
			if ((strstr(cbuffer, "<option value=\"") != NULL) &&
				(strstr(cbuffer, " name=\"") != NULL))
			{
				char*			pStart(strstr(cbuffer, "value="));
				char*			pEnd  (strstr(cbuffer, "\" name="));
				stringstream	sStream;
				unsigned short	value (0);
				bool			doSkip(false);

				//  read material code
				value = (unsigned short) atoi(pStart + 7);

				//  parse and read definition name
				pStart = pEnd + 8;
				pEnd   = strstr(pStart, "\">");
				*pEnd  = 0;
				if ((value > BP_BRAIN) && (strncmp(pStart, "BP_", 3) == 0))	continue;
				txtDefine = pStart;

				//  parse and read user readable name
				pStart = pEnd + 2;
				pEnd   = strstr(pStart, "</option>");
				*pEnd  = 0;
				if (strncmp(pStart, "Skyrim, ", 8) == 0)	pStart += 8;
				txtName = pStart;
				if (txtName.size() > 30)	txtName = txtName.substr(0, 30);

				sStream << txtName << "  [" << txtDefine << "]";

				//  append material to map
				_bodyPartMap[value] = sStream.str();

				sStream.str("");
				sStream.clear();
				sStream << value << " => " << txtDefine << "  [" << txtName << "]";

				_userMessages.push_back("^3added: " + sStream.str());
			}
			//  early break at end of material definition
			else if (strstr(cbuffer, "</enum>") != NULL)
			{
				break;
			}
		}  //  if (isBdySection)
	}  //  while (streamIn.good())

	sprintf(cbuffer, "^%cbody parts found: %d", ((_bodyPartMap.size() > 0) ? '2' : '5'), _bodyPartMap.size());
	_userMessages.push_back(cbuffer);

	streamIn.close();
}

//-----  reset()  -------------------------------------------------------------
void NifUtlMaterialList::reset()
{
	_userMessages.clear();
	_materialMap.clear();
}

//-----  getMaterialCode()  ---------------------------------------------------
unsigned int NifUtlMaterialList::getMaterialCode(string matDefName)
{
	return (_materialMap.count(matDefName) > 0) ? _materialMap[matDefName]._code : 0;
}

//-----  getMaterialName()  ---------------------------------------------------
string NifUtlMaterialList::getMaterialName(unsigned int material)
{
	for (auto pIter=_materialMap.begin(), pEnd=_materialMap.end(); pIter != pEnd; ++pIter)
	{
		if (pIter->second._code == material)	return pIter->second._name;
	}
	return "-unknown-";
}

//-----  getMaterialDefName()  ------------------------------------------------
string NifUtlMaterialList::getMaterialDefName(unsigned int material)
{
	for (auto pIter=_materialMap.begin(), pEnd=_materialMap.end(); pIter != pEnd; ++pIter)
	{
		if (pIter->second._code == material)	return pIter->second._defName;
	}
	return "-UNKNOWN-";
}

//-----  getBodyPartName()  ---------------------------------------------------
string NifUtlMaterialList::getBodyPartName(unsigned short index)
{
	return (_bodyPartMap.count(index) > 0) ? _bodyPartMap.at(index) : "-UNKNOWN-";
}