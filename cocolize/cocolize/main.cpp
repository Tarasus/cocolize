//
//  main.cpp
//  cocolize
//
//  Created by MacBook Pro on 31/07/12.
//  Copyright (c) 2012 Tall Developments. All rights reserved.
//
//  Edit by Tarasus 01.06.2021

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <sys/stat.h>
#include "tinyxml.h"

int verbose = 0;

std::string extractLanguage(std::__fs::filesystem::path pathobj)
{
    if(pathobj.parent_path().empty())
    {
//        return "";
    }
    auto parent = pathobj.parent_path();
    //printf("parent [%s]\n", parent.filename().string().c_str());
    
    if(parent.filename().string() == "values")
    {
        return "en";
    }
    
    std::stringstream stream(parent.filename().string());
    
    std::string segment;
    std::string result;
    
    while (std::getline(stream, segment, '-'))
    {
        if(segment == "values")
        {
            continue;
        }
        result += segment + "-";
    }
    if(result.empty())
    {
        return "";
    }
    result.pop_back();
    return result;
}


bool convertFile( const char* inFile, const char* outFile)
{
    if( inFile == NULL || outFile == NULL )
    {
        if(verbose)
        {
            printf("ERROR: inFile == NULL or outFile == NULL \n");
        }
        return false;
    }
    
    TiXmlDocument XMLdoc(inFile);
    bool loadOkay = XMLdoc.LoadFile();
    if( loadOkay ) {
        if(verbose)
        {
            printf("Successfully loaded file: [%s]\n", inFile);
        }
        TiXmlElement* pRoot = XMLdoc.FirstChildElement("resources");
        if( pRoot ) {
            
            std::ofstream outFileStream;
            outFileStream.open(outFile);
            if(!outFileStream.is_open())
            {
                if(verbose)
                {
                    printf("Failed opening stream at [%s]\n", outFile);
                }
                return false;
            }
            TiXmlElement* pString = pRoot->FirstChildElement("string");
            
            while (pString) {
                
                const char* key = pString->Attribute("name");
                const char* value = pString->GetText();
                
                if(key == NULL)
                {
                    outFileStream << "//ERR: KEYLESS VALUE "<< value <<"\n";
                }
                else if( value == NULL)
                {
                    outFileStream << "//ERR: NO VALUE FOR KEY "<< key <<"\n";
                }
                else
                {
                    //outFileStream << "/* No description supplied by the engineer */\n";
                    outFileStream << "\"" << key << "\" = \"" << value << "\";\n";
                }
                
                pString = pString->NextSiblingElement();
            }
            
            outFileStream.close();
        }
        else
        {
            if(verbose)
            {
                printf("Could not find document root 'resources' in file: [%s]\n",inFile);
            }
            return false;
        }
        
    }
    else
    {
        if(verbose)
        {
            printf("Could not open: [%s]! Confirm the file exists and is a valid android strings file\n",inFile);
        }
        return false;
    }
    return true;
}

bool convertFile( std::string in, std::string out)
{
    if(verbose)
    {
        printf("convertFile: [%s] -> [%s] \n",in.c_str(), out.c_str());
    }
    return convertFile(in.c_str(), out.c_str());
}

bool dirRoutine(const char* inDirPath, const char* outDirPath)
{
    std::string outPathMain;
    
    if(outDirPath == NULL)
    {
        outPathMain = std::string(inDirPath);
        outPathMain.append("/CONVERTED");
    }
    else
    {
        outPathMain = std::string(outDirPath);
    }
    
    bool dir = mkdir(outPathMain.c_str(), 0777);
    
    if(dir)
    {
        printf("Successfully created output dir = [%s]\n",outPathMain.c_str());
    }
    else
    {
        if(verbose)
        {
            printf("Failed creating dir at = [%s]\n",outPathMain.c_str());
        }
        return false;
    }
    
    
    using recursive_directory_iterator = std::__fs::filesystem::recursive_directory_iterator;
    
    int allfiles = 0;
    int xmlfiles = 0;
    int successconverts = 0;
    
    for(const auto& dirEntry : recursive_directory_iterator(inDirPath))
    {
        if(dirEntry.is_directory())
        {
            continue;
        }
        else
        {
            auto pathobj = dirEntry.path();
            std::string path = pathobj.string();
            std::string filename = pathobj.filename().string();
            std::string format = pathobj.extension().string();
            
            if(pathobj.parent_path().filename().string() == "CONVERTED")
            {
                continue;
            }
            
            allfiles++;
            
            if(format != ".xml")
            {
                if(verbose)
                {
                    printf("skipping invalid file [%s]\n", path.c_str());
                }
                continue;
            }
            
            xmlfiles++;
            
            std::string lang = extractLanguage(pathobj);
            if(lang.empty())
            {
                if(verbose)
                {
                    printf("failed extracting lang for = [%s] , falling back to filename\n", path.c_str() );
                }
                lang = filename;
            }
            
            bool res = convertFile(path, outPathMain+"/"+lang);
            if(res)
            {
                successconverts++;
            }
            else if(verbose)
            {
                printf("failed converting file [%s]\n",path.c_str());
            }
            
        }
    }
    
    printf("DIR CONVERSION FINISHED\n");
    printf("-----------------------\n");
    printf("INPUT  DIR [%s]\n",inDirPath);
    printf("OUTPUT DIR [%s]\n",outPathMain.c_str());
    printf("-----------------------\n");
    printf("Detected   : %d files\n",allfiles);
    printf("Out of them\n");
    printf("Valid .xml : %d files\n",xmlfiles);
    printf("Converted  : %d files\n",successconverts);
    printf("Failed     : %d files\n",xmlfiles - successconverts);
    printf("-----------------------\n");
    
    return true;
}

void printHelp()
{
    printf("Cocolize android xml to IOS localized strings converter.\n");
    printf("-----------------------\n");
    printf("-dir mode :recurses input directory searching for xml files.\n");
    printf("If xml file is located in \"values-[iso_langcode]\" subfolder it extracts iso_langcode and uses it as filename for resulting file.\n");
    printf("-----------------------\n");
    printf("Usage:\n");
    printf("-v or --verbose parameter turns on verbose mode\n");
    printf("\"cocolize -help\" : this manual.\n");
    printf("\"cocolize [-v] -dir <input directory> <output directory>\" : directory recursive mode.\n");
    printf("\"cocolize [-v] -dir <input directory>\" : directory recursive mode but generates CONVERTED subdir in input directory.\n");
    printf("\"cocolize [-v] <input file> <output file>\" : single file mode.\n");
}

int main(int argc, const char * argv[])
{
    //printf("Exec path = [%s]\n",argv[0]);
    
    if(argc <= 1)
    {
        printHelp();
        return 0;
    }
    
    if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-help") == 0)
    {
        printHelp();
        return 0;
    }
    
    if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--verbose") == 0)
    {
        verbose = 1;
        printf("VERBOSE MODE\n");
    }
    
    if(strcmp(argv[1+verbose], "-dir") == 0)
    {
        const char* inDirPath = argv[2+verbose];
        const char* outDirPath = argv[3+verbose];
        if(!dirRoutine(inDirPath, outDirPath))
        {
            printf("Dir conversion failed, try -v or --verbose\n");
        }
        return 0;
    }
    
    const char* inFilePath  = argv[1+verbose];
    const char* outFilePath = argv[2+verbose];
    if(!convertFile(inFilePath, outFilePath))
    {
        printf("File conversion failed, try -v or --verbose\n");
    }
    
    return 0;
}




