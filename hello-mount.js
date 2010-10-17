
var mp = process.argv[2]
  , args = process.argv.slice(3)
  , child_process = require("child_process")
if (!mp) throw new Error("usage: node "+process.argv[1]+" <mountpoint> [args]")

require("./build/default/hello-ll").mount(mp, args, function (er) {
  console.error(er && er.stack || "mounted and unmounted ok")
})

// kludge, need to make it fire the cb on init rather than unmount
setTimeout(function () {
  require("fs").readFile(require("path").join(mp, "hello"), function (er, hw) {
    if (er) throw er
    console.log(hw.toString())
  })
}, 200)
