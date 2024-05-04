#include "Wad.h"
#include <algorithm>
#include <cstring>
#include <regex>

// Creates a new Directory instance
Directory* newDir(string name, bool isDir, Directory* parent) {
    Directory* dir = new Directory; // Allocate new Directory structure
    dir->name = name;
    dir->isDir = isDir;
    dir->parent = parent;
    return dir; // Return the new directory structure
};

// Loads WAD data from the specified file path into a new Wad object.
Wad* Wad::loadWad(const string &path) {
    fstream file(path, ios_base::in|ios_base::binary); // Open the file in binary read mode
    Wad* wad = new Wad(); // Create a new Wad instance
    wad->root = newDir("/", true, nullptr); // Initialize the root directory
    wad->root_file = path; // Store the file path
    // Read the magic header from the file
    for(int i = 0; i < 4; ++i) {
        file.read(&wad->magic[i], 1);
    }
    string magic(wad->magic);
    // Check if the magic header is valid
    if(magic.find("WAD") == string::npos) {
        return nullptr; // Return null if invalid
    }
    // Read descriptor count and offset from the file
    file.read((char*)&wad->numDesc, sizeof(int));
    file.read((char*)&wad->descOffset, sizeof(int));
    // Move to the descriptor section in the file
    file.seekg(wad->descOffset, file.beg);
    // Read each descriptor and build the structure
    for(unsigned int i = 0; i < wad->numDesc; ++i) {
        Descriptor desc;
        file.read((char*)&desc.elemOffset, sizeof(int));
        file.read((char*)&desc.elemLength, sizeof(int));
        char c[8];
        bool flag = true;
        // Read the descriptor name
        for(int i = 0; i < 8; ++i) {
            file.read(&c[i], 1);
            if (c[i] != '\0'&&flag) {
                desc.name += c[i];
            }
            else
                flag = false;
        }
        wad->descriptors.push_back(desc);
    }
    int mapCount = 10;
    bool inMap = false;
    Directory* cur = wad->root;
    unsigned int e = wad->descriptors.size();
    // Process each descriptor to build directories and files
    for(unsigned int i = 0; i < wad->descriptors.size(); ++i) {
        file.seekg(wad->descriptors[i].elemOffset, ios_base::beg);
        bool isMap =
                (wad->descriptors[i].name[0] == 'E'
                 && isdigit(wad->descriptors[i].name[1])
                 && wad->descriptors[i].name[2] == 'M'
                 && isdigit(wad->descriptors[i].name[3]));
        string curName(wad->descriptors[i].name);
        bool startNamespace = (curName.find("_START") != string::npos);
        bool endNamespace = (curName.find("_END") != string::npos);
        // Handle map and namespace markers
        if(mapCount == 0) {
            mapCount = 10;
            inMap = false;
            cur = cur->parent; // Move up to parent directory
        }
        if(isMap) {
            cur = newDir(curName, true, cur); // Create new map directory
            inMap = true;
            cur->parent->childDir.push_back(cur); // Add to parent's child list
            continue;
        } else if(startNamespace) {
            string name = wad->descriptors[i].name;
            name.erase(name.find("_START"), string::npos);
            cur = newDir(name, true, cur); // Create new namespace directory
            cur->parent->childDir.push_back(cur); // Add to parent's child list
            continue;
        } else if(endNamespace) {
            cur = cur->parent; // End namespace, move up to parent
            continue;
        }
        // Create file entries within maps and namespaces
        if(inMap) {
            Directory* dir = newDir(wad->descriptors[i].name, false, cur);
            char c[wad->descriptors[i].elemLength];
            string s = "";
            for(unsigned int j = 0; j < wad->descriptors[i].elemLength; ++j) {
                file.read(&c[j], 1);
                s += c[j];
            }
            dir->lump = s; // Store file data in lump
            cur->childDir.push_back(dir); // Add to parent's child list
            mapCount--;
        } else {
            Directory* dir = newDir(wad->descriptors[i].name, false, cur);
            char c[wad->descriptors[i].elemLength];
            string s = "";
            for(unsigned int j = 0; j < wad->descriptors[i].elemLength; ++j) {
                file.read(&c[j], 1);
                s += c[j];
            }
            dir->lump = s; // Store file data in lump
            cur->childDir.push_back(dir); // Add to parent's child list
        }
    }
    file.close(); // Close the file
    return wad; // Return the loaded Wad instance
}

// Returns the magic identifier of the WAD file.
string Wad::getMagic() {
    return string (this->magic, 4);
}

// Determines if the specified path points to content (data) within the WAD file.
bool Wad::isContent(const string &path) {
    Directory* cur = this->root; // Start at the root directory
    stringstream s(path);
    string dir;
    // Traverse the path by parsing each segment
    while(getline(s, dir, '/')) {
        if(dir.empty())
            continue;
        // Find the directory or file within the current directory
        for(unsigned int i = 0; i < cur->childDir.size(); ++i) {
            if(cur->childDir[i]->name == dir) {
                cur = cur->childDir[i];
            }
        }
    }
    // Check if the final destination is a file and not a directory
    if(!cur->isDir) {
        return true;
    }
    // If a segment is not found, it's not a valid path
    return false;
}

// Determines if the specified path points to a directory.
bool Wad::isDirectory(const string &path) {
    if (path.empty()) {
        return false; // An empty path is not valid
    }
    Directory* cur = this->root; // Start at the root directory
    stringstream pathStream(path);
    string segment;
    vector<string> segments;
    // Split the path into segments
    while (getline(pathStream, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    if (segments.empty()) {
        return true; // If no segments, it's the root directory
    }
    // Traverse the segments to find the specified directory
    for (size_t i = 0; i < segments.size(); ++i) {
        bool found = false;
        for (auto& child : cur->childDir) {
            if (child->name == segments[i]) {
                cur = child;
                found = true;
                break;
            }
        }
        if (i == segments.size() - 1 && found) {
            return cur->isDir; // If the last segment matches and it's a directory
        } else if (!found) {
            return false; // If any segment does not match, it's not a valid directory
        }  
    }
    return false; // Should not reach here
}

// Returns the size in bytes of the content at the specified path, or -1 if it's a directory.
int Wad::getSize(const string &path) {
    if(isContent(path)) { // Check if the path points to content
        Directory* cur = this->root; // Start at the root directory
        stringstream s(path);
        string dir;
        // Traverse the path by parsing each segment
        while(getline(s, dir, '/')) {
            if(dir.empty()) {
                continue;
            }
            // Find the directory or file within the current directory
            for(unsigned int i = 0; i < cur->childDir.size(); ++i) {
                if(cur->childDir[i]->name == dir) {
                    cur = cur->childDir[i];
                    break;
                }
            }
        }
        // Return the size of the content
        return cur->lump.size();
    }
    return -1; // Return -1 if the path does not point to content or is invalid
}

// Copies content data into a buffer, starting at the specified offset.
int Wad::getContents(const string &path, char *buffer, int length, int offset) {
    if(isContent(path)) { // Check if the path points to content
        Directory* cur = this->root; // Start at the root directory
        stringstream s(path);
        string dir;
        // Traverse the path by parsing each segment
        while(getline(s, dir, '/')) {
            for(unsigned int i = 0; i < cur->childDir.size(); ++i) {
                if(cur->childDir[i]->name == dir) {
                    cur = cur->childDir[i]; // Navigate to the next directory or file
                }
            }
        }
        int buffStart = 0; // Buffer start index
        if(offset>cur->lump.size()) {
            cerr<<"Offset too large"<<endl; // Handle offset out of bounds error
            return 0;
        }
        else if (offset+length > cur->lump.size()) {   
            length = length-(offset+length-cur->lump.size()); // Adjust length if it exceeds file size
        }
        for(int i = offset; i < length+offset; i++) {
            buffer[buffStart] = cur->lump[i]; // Copy data into buffer
            buffStart++;
        }
        return buffStart; // Return number of bytes copied
    }
    return -1; // Return -1 if the path does not point to content
}

// Retrieves the list of entries in the specified directory.
int Wad::getDirectory(const string &path, vector<string> *directory) {
    if(isDirectory(path)) { // Check if the path points to a directory
        Directory* cur = this->root; // Start at the root directory
        stringstream s(path);
        string dir;
        // Traverse the path by parsing each segment
        while(getline(s, dir, '/')) {
            if(dir.empty())
                continue;
            for(unsigned int i = 0; i < cur->childDir.size(); ++i) {
                if(cur->childDir[i]->name == dir) {
                    cur = cur->childDir[i]; // Navigate to the next directory
                }
            }
        }
        int numEntries = 0;
        for(unsigned int i = 0; i < cur->childDir.size(); ++i) {
            directory->push_back(cur->childDir[i]->name); // Add directory name to the output vector
            numEntries++;
        }
        return numEntries; // Return the number of entries found
    }
    return -1; // Return -1 if the path does not point to a directory
}


bool matchesPattern(const std::string& input) {
    std::regex pattern("E\\d*M\\d*"); // Regex to match map marker patterns like "E1M1"
    return std::regex_match(input, pattern); // Returns true if input matches the pattern
}

// Creates a new directory at the specified path using namespace markers.
void Wad::createDirectory(const string &path) {
    stringstream s(path); // Stream to parse the path
    string dirName;
    vector<string> pathComponents; // To store each component of the path
    // Split the path into components
    while (getline(s, dirName, '/')) {
        pathComponents.push_back(dirName);
    }
    auto it_start = this->descriptors.begin(); // Iterator to track the start of the descriptor list
    auto it_end = this->descriptors.end(); // Iterator to track the end of the descriptor list
    Directory* parent = this->root; // Start from the root directory
    for (size_t i = 1; i < pathComponents.size(); i++) {
        bool found = false;
        // Prevent creating directories with map marker names
        if(i==1 && matchesPattern(pathComponents[i])) {
            cerr<<"Cannot create EM"<<endl;
            break;
        }
        // Traverse the existing directory structure
        for (auto& child : parent->childDir) {
            if (child->name == pathComponents[i]) {
                parent = child;
                found = true;
                // Update iterators to reflect namespace boundaries
                string target = child->name+"_START";
                it_start = std::find_if(it_start, it_end,[&target](const Descriptor& dir) { return dir.name == target; });
                target = child->name+"_END";
                it_end = std::find_if(it_start, it_end,[&target](const Descriptor& dir) { return dir.name == target; });
                break;
            }
        }
        // Handle the creation of a new directory
        if (!found && i==pathComponents.size()-1) {
            if(pathComponents[i].size()>2) {
                cerr<<"Too long directory name"<<endl;
                return;
            }
            // Create and insert the new directory
            Directory* newDirectory = newDir(pathComponents[i], true, parent);
            auto it = find_if(parent->childDir.begin(), parent->childDir.end(), [](const Directory* dir) {
                return dir->name.find("_END") != string::npos;
            });
            parent->childDir.insert(it, newDirectory);
            if(i==1) {
                // Insert namespace start and end markers into the descriptor list
                Descriptor newDescriptor;
                newDescriptor.elemOffset = 0;
                newDescriptor.elemLength = 0;
                newDescriptor.name = newDirectory->name+"_START";
                descriptors.push_back(newDescriptor);
                newDescriptor.elemOffset = 0;
                newDescriptor.elemLength = 0;
                newDescriptor.name = newDirectory->name+"_END";
                descriptors.push_back(newDescriptor);
                parent = newDirectory;
            }
            else {
                Descriptor newDescriptor;
                newDescriptor.elemOffset = 0;
                newDescriptor.elemLength = 0;
                newDescriptor.name = newDirectory->name+"_START";
                string target = parent->name+"_END";
                if (it_start != this->descriptors.end()) {
                    this->descriptors.insert(it_end, newDescriptor);
                }
                newDescriptor.elemOffset = 0;
                newDescriptor.elemLength = 0;
                newDescriptor.name = newDirectory->name+"_END";
                if (it_end != this->descriptors.end()) {
                    this->descriptors.insert(it_end+1, newDescriptor);
                }
                parent = newDirectory;
            }
        }
        else if (found)
            continue;
        else {
            cerr<<"No such parent path"<<endl;
            break;
        }
        found = false;
        }
    this->wadSave(this->root_file); // Save changes to the WAD file
}

// Creates a new file at the specified path with zero length and offset.
void Wad::createFile(const string &path) {
    stringstream s(path);
    string dirName;
    vector<string> pathComponents;
    auto it_start = this->descriptors.begin();
    auto it_end = this->descriptors.end();
    // Parses the input path and divides it into components.
    while (getline(s, dirName, '/')) {
        pathComponents.push_back(dirName);
    }
    Directory* parent = this->root;
    // Begins from the root directory and navigates through the path.
    for (size_t i = 1; i < pathComponents.size() - 1; i++) {
        bool found = false;
        for (auto& child : parent->childDir) {
            if (child->name == pathComponents[i]) {
                parent = child;
                found = true; // Updates the parent to the current directory.
                string target = child->name+"_START";
                
                it_start = std::find_if(it_start, it_end,[&target](const Descriptor& dir) { return dir.name == target; });
                target = child->name+"_END";
                it_end = std::find_if(it_start, it_end,[&target](const Descriptor& dir) { return dir.name == target; });
                break;
            }
        }
        if (!found) {
            cerr << "Invalid path: Parent directory does not exist" << endl;
            return; // Exits if the path is incorrect or the directory does not exist.
        }
    }
    // Ensures that files are not created under map markers, respecting the constraints of the WAD format.
    if (matchesPattern(pathComponents[1])) {
        cerr << "Cannot create a file inside a map marker." << endl;
        return;
    }
    // Validates the length of the file name.
    if(pathComponents.back().size()>8) {
        cerr<<"Too long"<<endl;
        return;
    }
    // Creates the file structure, sets its location, and inserts it into the parent directory.
    Directory* newFile = newDir(pathComponents.back(), false, parent);
    auto it = find_if(parent->childDir.begin(), parent->childDir.end(), [](const Directory* dir) {
        return dir->name.find("_END") != string::npos;
    });
    parent->childDir.insert(it, newFile);
    // Updates the descriptor list with the new file's details.
    Descriptor newDescriptor;
    newDescriptor.elemOffset = 0;
    newDescriptor.elemLength = 0;
    newDescriptor.name = pathComponents.back();
    string target = parent->name+"_END";
    auto itD = std::find_if(this->descriptors.begin(), this->descriptors.end(),[&target](const Descriptor& dir) { return dir.name == target; });
    if(it_start!=this->descriptors.end()) {
        this->descriptors.insert(it_end,newDescriptor);
    }
    else {
        this->descriptors.push_back(newDescriptor);
    }
    this->wadSave(this->root_file); // Saves the WAD after changes.
}

// Writes data to a file in the WAD file at the specified path.
int Wad::writeToFile(const string &path, const char *buffer, int length, int offset ) {
    stringstream s(path);
    string dirName;
    Directory* cur = this->root;
    // Parses the input path and finds the target file.
    while (getline(s, dirName, '/')) {
        if(dirName.empty()) {
            continue;
        }
        bool found = false;
        for (auto& child : cur->childDir) {
            if (child->name == dirName) {
                cur = child;
                found = true;
                break;
            }
        }
        if (!found) {
            cerr << "File not found." << endl;
            return -1; // File not found error.
        }
    }
    // Validates that the target is a file and not a directory.
    if (cur->isDir) {
        cerr << "The specified path is a directory, not a file." << endl;
        return -1;
    }
    // Checks if there's pre-existing data that shouldn't be overwritten.
    if(cur->lump.size()>0) {
        cerr<<"Existing files"<<endl;
        return 0;
    }
    // Checks if the offset is valid.
    if (offset > cur->lump.size()) {
        cerr << "Offset is beyond the current file size." << endl;
        return -1;
    }
    // Writes the buffer to the file at the specified offset and length.
    cur->lump.replace(offset, length, buffer, length);
    // Updates the descriptor for the file.
    for (auto& desc : descriptors) {
        if (desc.name == dirName) {
            desc.elemOffset = 0;
            desc.elemLength = cur->lump.size();
            break;
        }
    }
    this->wadSave(this->root_file); // Saves the WAD after changes.
    return length; // Returns the number of bytes written.
}

// Recursively searches for a directory by name starting from curDir.
Directory* Wad::findDirectoryRecursive(Directory* curDir, const string &name) {
    if (curDir->name == name) return curDir; // Base case: if the current directory's name matches the target name, return it.
    // Recursive case: search through each child directory.
    for (auto& child : curDir->childDir) {
        Directory* found = findDirectoryRecursive(child, name);
        if (found) return found;
    }
    return nullptr; // If no directory is found, return nullptr.
}

// Saves the current WAD structure to the file at the specified path. 
bool Wad::wadSave(const string &path) {
    fstream file(path, ios_base::out|ios_base::binary); // Opens the file for binary writing.
    if (!file.is_open()) {
        cerr << "Failed to open file for writing: " << path << endl;
        return false; // Exit if the file could not be opened.
    }
    file.write(this->magic, 4); // Writes the magic header to the file.
    unsigned int numDesc = descriptors.size();
    file.write((char*)&numDesc, sizeof(numDesc)); // Writes the number of descriptors.
    unsigned int descOffset = sizeof(magic) + 2 * sizeof(numDesc);
    file.write((char*)&descOffset, sizeof(descOffset)); // Calculates and writes the descriptor offset from the start of the file.
    file.seekp(descOffset + numDesc * sizeof(Descriptor)); // Sets the file pointer to the position where descriptors start.
    // Writes each descriptor's data to the file.
    for (auto &desc : descriptors) {
        Directory* dir = findDirectoryRecursive(this->root, desc.name);
        // If the directory is a file (not a directory) and it has data, write it to the file.
        if (dir && !dir->isDir) {
            desc.elemOffset = file.tellp(); // Update the offset to the current position in the file.
            desc.elemLength = dir->lump.size();  // Update the length to the size of the data.
            file.write(dir->lump.data(), dir->lump.size()); // Write the data.
        } 
        else {
            desc.elemOffset = 0; // No offset for directories or empty files.
            desc.elemLength = 0; // No length for directories.
        }
    }
    file.seekp(descOffset); // Rewinds to the descriptor offset to write the updated descriptors.
    for (const auto &desc : descriptors) {
        file.write((char*)&desc.elemOffset, sizeof(desc.elemOffset)); // Write the offset of the data.
        file.write((char*)&desc.elemLength, sizeof(desc.elemLength)); // Write the length of the data.
        file.write(desc.name.c_str(), 8); // Write the name, ensuring it is no more than 8 characters.
    }
    file.close(); // Closes the file.
    return true; // Returns true to indicate successful saving.
}