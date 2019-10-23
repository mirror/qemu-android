// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <cstdint>
#include <ostream>
#include <string>

namespace android {
namespace emulation {
namespace control {

enum TarType {
    REGTYPE = 0,   // regular file
    LNKTYPE = 1,   // link (inside tarfile)
    SYMTYPE = 2,   // symbolic link
    CHRTYPE = 3,   // character special device
    BLKTYPE = 4,   // block special device
    DIRTYPE = 5,   // directory
    FIFOTYPE = 6,  // fifo special device
    CONTTYPE = 7,  // contiguous file
};

enum HeaderTypes : char {
    XHDTYPE = 'x',           // POSIX.1-2001 extended header
    XGLTYPE = 'g',           // POSIX.1-2001 global header
    SOLARIS_XHDTYPE = 'X',   // Solaris extended header
    GNUTYPE_LONGNAME = 'L',  // GNU tar longname
    GNUTYPE_LONGLINK = 'K',  // GNU tar longlink
    GNUTYPE_SPARSE = 'S'     // GNU tar sparse file
};

enum class TarFormat {
    USTAR_FORMAT = 0,  // POSIX.1-1988 (ustar) format
    GNU_FORMAT = 1,    // GNU tar format
    PAX_FORMAT = 2     // POSIX.1-2001 (pax) format
};

enum TarError {
    TAR_OK = 0,
    TAR_INVALID_INFO,
    TAR_STREAM_NO_SEEK,
    TAR_INVALID_HEADER,
};

// MAGIC USTAR HEADER
#define USTAR "ustar "
#define TARBLOCK 512

/* POSIX header.  */
using octalnr = char;

/* tar Header Block, from POSIX 1003.1-1990.
     should be exactly the size of a tarblock.
 */
struct posix_header {    /* byte offset */
    char name[100];      /*   0 */
    octalnr mode[8];     /* 100 */
    octalnr uid[8];      /* 108 */
    octalnr gid[8];      /* 116 */
    octalnr size[12];    /* 124 */
    octalnr mtime[12];   /* 136 */
    octalnr chksum[8];   /* 148 */
    uint8_t typeflag;    /* 156 */
    char linkname[100];  /* 157 */
    char magic[6];       /* 257, should contain "ustar\0" */
    octalnr version[2];  /* 263 */
    char uname[32];      /* 265 */
    char gname[32];      /* 297 */
    octalnr devmajor[8]; /* 329 */
    octalnr devminor[8]; /* 337 */
    octalnr prefix[155]; /* 345 */
    octalnr padding[12]; /* 500 */
    /* 512 */
};
static_assert(sizeof(posix_header) == TARBLOCK,
              "Size of the tar header should be 512 bytes");

// A class that can add files/directories to a stream as tar objects.
// The tar entries will be written using the ustar fromat.
// Only files and directories are supported (no symlinks).
// And filenames up to 99 characters in length.
//
// Make sure to call close when you are finished adding files.
class TarWriter {
public:
    TarWriter(std::string cwd, std::ostream& dest) : mDest(dest), mCwd(cwd) {}
    bool addFileEntry(std::string fname);
    bool addDirectoryEntry(std::string dname);
    bool addDirectory(std::string dname);

    // Closes the tar archive and flushes the underlying stream.
    bool close();

private:
    bool writeTarHeader(std::string name);
    std::ostream& mDest;
    std::string mCwd;
    bool mClosed = false;
};

struct TarInfo {
    std::string name;        // member name
    mode_t mode = 0644;      // file permissions
    uid_t uid = 0;           // user id
    gid_t gid = 0;           // group id
    uint64_t size = 0;       // file size
    uint64_t mtime = 0;      // modification time
    TarType type = REGTYPE;  // member type
    std::string uname = "";  // user name
    std::string gname = "";  // group name
    uint64_t offset = 0;     // offset in stream
    bool valid = false;      // True if this is a valid entry.
    TarError error = TAR_OK; // Error code (if valid == false).
};

class TarReader {
public:
    TarReader(std::string cwd, std::istream& src) : mSrc(src), mCwd(cwd) {}
    TarInfo next(TarInfo from);
    TarInfo first() { return next({}); }
    bool extract(TarInfo src);

private:
    std::istream& mSrc;
    std::string mCwd;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
