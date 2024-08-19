header = '''
@@require(PLATFORM, VERSION, isTX, hasSubGHz, is8285)
<!DOCTYPE HTML>
<html lang="en">

<head>
	<title>AM32 Configurator</title>
	<meta charset="utf-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1" />
	<script>
let inline_loader_list = [
	{ tag: 'link'  , attribs: { rel: 'stylesheet', href: 'elrs.css'}},
	{ tag: 'script', attribs: { src: 'mui.js'}},
    { tag: 'script', attribs: { src: 'libs.js'}},
    { tag: 'script', attribs: { src: 'am32.js'}},
    { tag: 'script', attribs: { src: 'ihex.js'}},
];
function inline_loader_ondone() {
	am32_init();
}
@@include("inlineseqloader.js")
	</script>
</head>
'''

with open("am32.html", "w") as fout:
    fout.write(header.strip())
    with open("am32_dev.html", "r") as fin:
        found_body = False
        for line in fin:
            if "<body" in line:
                found_body = True
                fout.write("<body>\n")
                continue
            if found_body:
                fout.write(line)
