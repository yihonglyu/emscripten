// Copyright 2021 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.
// This file defines the file object of the new file system.
// Current Status: Work in Progress.
// See https://github.com/emscripten-core/emscripten/issues/15041.

#include "file.h"
#include "wasmfs.h"
#include <emscripten/threading.h>

namespace wasmfs {
//
// Directory
//
std::shared_ptr<File> Directory::Handle::getEntry(std::string pathName) {
  auto it = getDir()->entries.find(pathName);
  if (it == getDir()->entries.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}
//
// Path Parsing utilities
//
ParsedPath getParsedPath(std::vector<std::string> pathParts,
                         long& err,
                         std::shared_ptr<File> forbiddenAncestor) {
  std::shared_ptr<Directory> curr;
  auto begin = pathParts.begin();

  if (pathParts.empty()) {
    err = -ENOENT;
    return ParsedPath{{}, nullptr};
  }

  // Check if the first path element is '/', indicating an absolute path.
  if (pathParts[0] == "/") {
    curr = wasmFS.getRootDirectory();
    begin++;
    // If the pathname is the root directory, return the root as the child.
    if (pathParts.size() == 1) {
      return ParsedPath{curr->locked(), curr};
    }
  } else {
    curr = wasmFS.getCWD();
  }

  for (auto pathPart = begin; pathPart != pathParts.end() - 1; ++pathPart) {
    // Find the next entry in the current directory entry
#ifdef WASMFS_DEBUG
    directory->locked().printKeys();
#endif
    auto entry = curr->locked().getEntry(*pathPart);

    if (forbiddenAncestor) {
      if (entry == forbiddenAncestor) {
        err = -EINVAL;
        return ParsedPath{{}, nullptr};
      }
    }

    // Requested entry (file or directory)
    if (!entry) {
      err = -ENOENT;
      return ParsedPath{{}, nullptr};
    }

    curr = entry->dynCast<Directory>();

    // If file is nullptr, then the file was not a Directory.
    // TODO: Change this to accommodate symlinks
    if (!curr) {
      err = -ENOTDIR;
      return ParsedPath{{}, nullptr};
    }

#ifdef WASMFS_DEBUG
    emscripten_console_log(*pathPart->c_str());
#endif
  }

  // auto parent = curr->dynCast<Directory>();

  auto child = curr->locked().getEntry(*(pathParts.end() - 1));
  return ParsedPath{curr->locked(), child};
}

std::shared_ptr<Directory> getDir(std::vector<std::string>::iterator begin,
                                  std::vector<std::string>::iterator end,
                                  long& err,
                                  std::shared_ptr<File> forbiddenAncestor) {

  std::shared_ptr<File> curr;
  // Check if the first path element is '/', indicating an absolute path.
  if (*begin == "/") {
    curr = wasmFS.getRootDirectory();
    begin++;
  } else {
    curr = wasmFS.getCWD();
  }

  for (auto it = begin; it != end; ++it) {
    auto directory = curr->dynCast<Directory>();

    // If file is nullptr, then the file was not a Directory.
    // TODO: Change this to accommodate symlinks
    if (!directory) {
      err = -ENOTDIR;
      return nullptr;
    }

    // Find the next entry in the current directory entry
#ifdef WASMFS_DEBUG
    directory->locked().printKeys();
#endif
    curr = directory->locked().getEntry(*it);

    if (forbiddenAncestor) {
      if (curr == forbiddenAncestor) {
        err = -EINVAL;
        return nullptr;
      }
    }

    // Requested entry (file or directory)
    if (!curr) {
      err = -ENOENT;
      return nullptr;
    }

#ifdef WASMFS_DEBUG
    emscripten_console_log(it->c_str());
#endif
  }

  auto currDirectory = curr->dynCast<Directory>();

  if (!currDirectory) {
    err = -ENOTDIR;
    return nullptr;
  }

  return currDirectory;
}

// TODO: Check for trailing slash, i.e. /foo/bar.txt/
// Currently any trailing slash is ignored.
std::vector<std::string> splitPath(char* pathname) {
  std::vector<std::string> pathParts;
  char newPathName[strlen(pathname) + 1];
  strcpy(newPathName, pathname);

  // TODO: Other path parsing edge cases.
  char* current;
  // Handle absolute path.
  if (newPathName[0] == '/') {
    pathParts.push_back("/");
  }

  current = strtok(newPathName, "/");
  while (current != NULL) {
    pathParts.push_back(current);
    current = strtok(NULL, "/");
  }

  return pathParts;
}
} // namespace wasmfs
