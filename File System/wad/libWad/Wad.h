#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <cctype>
#include <sstream>

using namespace std;

struct Directory {
    string name; // name of the directory
    bool isDir; // indicating whether this is a directory (true) or a file (false)
    string lump; // containing the data
    Directory* parent; // pointer to the parent directory
    vector<Directory*> childDir; // vector of pointers to child directories or files
};

Directory* newDir(string name, bool isDir, Directory* parent); // Creates a new Directory instance

class Wad {
private:
    struct Descriptor {
        unsigned int elemOffset = 0; // The offset in the WAD file where the file/directory begins
        unsigned int elemLength = 0; // The length of the file/directory data in bytes
        string name = ""; // The name of the file/directory
    };

    char magic[4]; // A character array to store the WAD file identifier (e.g., "IWAD" or "PWAD")
    unsigned int numDesc = 0; // The number of descriptors in the WAD file
    unsigned int descOffset = 0; // The offset in the WAD file where the descriptor list begins

public:
    vector<Descriptor> descriptors; // A vector of Descriptor structs that describe each element in the WAD file.
    string root_file; // A string representing the path to the root file
    Directory* root; // A pointer to the root directory structure

    static Wad* loadWad(const string &path); // Loads WAD data from the specified file path into a new Wad object.
    string getMagic(); // Returns the magic identifier of the WAD file.
    bool isContent(const string &path); // Determines if the specified path points to content (data) within the WAD file.
    bool isDirectory(const string &path); // Determines if the specified path points to a directory.
    int getSize(const string &path); // Returns the size in bytes of the content at the specified path, or -1 if it's a directory.
    int getContents(const string &path, char *buffer, int length, int offset = 0); // Copies content data into a buffer, starting at the specified offset.
    int getDirectory(const string &path, vector<string> *directory); // Retrieves the list of entries in the specified directory.
    void createDirectory(const string &path); // Creates a new directory at the specified path using namespace markers.
    void createFile(const string &path); // Creates a new file at the specified path with zero length and offset.
    int writeToFile(const string &path, const char *buffer, int length, int offset = 0); // Writes data to a file in the WAD file at the specified path.
    Directory* findDirectoryRecursive(Directory* curDir, const string &name); // Recursively searches for a directory by name starting from curDir.
    bool wadSave(const string &path); // Saves the current WAD structure to the file at the specified path.    
};