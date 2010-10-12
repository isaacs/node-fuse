var fuse = require("./fuse")
  , constants = require("constants")
fuse.mount
  ( { getattr : function (path, cb) {
        if (path === "/") {
          cb( null
            , { st_mode : constants.S_IFDIR | 0755
              , st_nlink : 2
              }
            )
        } else if (path === "/hello") {
          cb(null
            , { st_mode : constants.S_IFREG | 0444
              , st_nlink : 1
              , st_size : "hello world".length
              }
            )
        } else {
          cb(-1 * constants.ENOENT)
        }
        return 0
      }
    , readdir : function (path, offset, info) {
        if (path !== "/") return -1 * constants.ENOENT
        return [ "." , ".." , "hello" ]
      }
    , open : function (path, info) {
        if (path !== "/hello") return -1 * constants.ENOENT
        if((info.flags & 3) != constants.O_RDONLY ) {
          return -1 * constants.EACCES
        }
        return 0
      }
    , read : function (path, buf, size, offset, info, cb) {
        
      }
    }
  , []
  )
