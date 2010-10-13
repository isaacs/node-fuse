
// warning: vapor!
// this is just an API sketch.
// the actual implementation may end up in a different place.



var fuse = require("./fuse")
  , constants = require("constants")
  , hello = "Hello, world!"
  , mountPoint = process.argv[2]

// if no mount point provided, throw
// if it's not a dir, then fuse will raise an error later.
if (!mountPoint) throw new Error("no mountpoint")

// mount the filesystem
fuse.mount
  ( { getattr : getattr
    , readdir : readdir
    , open : open
    , read : read
    }
  , mountPoint // the folder to mount onto
  , [] // args to fuse
  )


function getattr (path, cb) {
  if (path === "/") {
    cb( null
      , { st_mode : constants.S_IFDIR | 0755
        , st_nlink : 2
        } // stat object
      )
  } else if (path === "/hello") {
    cb(null
      , { st_mode : constants.S_IFREG | 0444
        , st_nlink : 1
        , st_size : hello.length
        } // stat object
      )
  } else {
    cb(constants.ENOENT) // error code
  }
}
function readdir (path, offset, info, cb) {
  if (path !== "/") {
    cb(constants.ENOENT)
  } else {
    cb(null, [ "." , ".." , "hello" ])
  }
}
function open (path, info, cb) {
  if (path !== "/hello") return cb(constants.ENOENT)
  if((info.flags & 3) != constants.O_RDONLY ) {
    cb(constants.EACCES) // readonly fs
  } else {
    cb() // pass in nothing, the fd is managed by fuse.
  }
}
function read (path, size, offset, info, cb) {
  if (path !== "/hello") return cb(constants.ENOENT)
  return cb(new Buffer(hello.slice(offset, size)))
}
