
var mp = process.argv[2]
  , args = process.argv.slice(3)
if (!mp) throw new Error("usage: node "+process.argv[1]+" <mountpoint> [args]")

require("./build/default/hello-ll").mount(mp, args, function (er) {
  console.error(er && er.stack || "ok")
})

console.log("if you see this before it ends, then async works.")
