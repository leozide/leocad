import glob

navbar = """<!-- navbar start -->
<div class="nav-main">
  <div class="wrap">
    <a class="nav-home" href="{root}index.html"><img class="nav-logo" src="{root}leocad-32.png" width="32" height="32"> LeoCAD </a>
    <ul class="nav-site nav-site-internal">
      <li><a href="{root}download.html">Download</a></li>
      <li><a href="{docs}start.html">Documentation</a></li>
<!--      <li><a href="{root}help.html">Help</a></li> -->
    </ul>
    <ul class="nav-site nav-site-external">
      <li><a href="https://github.com/leozide/leocad">GitHub</a></li>
    </ul>
  </div>
</div>
"""

docbar = """<!-- docbar start -->
<div class="nav-docs">
  <div class="nav-docs-section">
    <h3>General Information</h3>
    <ul>
      <li><a href="start.html">Getting Started</a></li>
      <li><a href="library.html">Parts Library</a></li>
      <li><a href="compiling.html">Compiling Guide</a></li>
      <li><a href="license.html">License</a></li>
      <li><a href="history.html">Version History</a></li>
    </ul>
  </div>
  <div class="nav-docs-section">
    <h3>Tutorials</h3>
    <ul>
<!--
      <li><a href="">Choosing and Adding Pieces</a></li>
      <li><a href="">Moving and Rotating Pieces</a></li>
      <li><a href="camera.html">Camera and Viewport</a></li> 
      <li><a href="">Models</a></li> 
      <li><a href="">Flexible Parts</a></li> 
      Keyframing
      POV-Ray
 -->
      <li><a href="tutorial1.html">Basic Tutorial</a></li>
      <li><a href="rotation.html">Rotation Center</a></li>
    </ul>
  </div>
<!--      <li><a href="">Coordinate System</a></li>
      <li><a href="">Configuration Shortcuts</a></li>
  -->
  <div class="nav-docs-section">
    <h3>Reference</h3>
    <ul>
      <li><a href="meta.html">Meta Commands</a></li>
      <li><a href="texmap.html">Texture Mapping</a></li>
    </ul>
  </div>
</div>
"""

footer = """<!-- footer start -->
<footer class="wrap"><div class="right"> &copy; LeoCAD.org </div></footer>
"""

def setActive(text, file):
	quotes = file + '"'
	return text.replace(quotes, quotes + ' class="active"')

def addNavBar(file):
	if file == 'download.html' or file == 'help.html':
		return setActive(navbar, file)
	elif file != 'index.html':
		return setActive(navbar, 'start.html')
	else:
		return navbar

def addDocBar(file):
	return setActive(docbar, file)

def addFooter(file):
    return footer

def processFile(folder, file):
	skip = False
	root = ''
	docs = 'docs/'
	if folder == 'docs/':
		root = '../'
		docs = ''

	with open(folder + file) as oldfile:
		lines = []
		for line in oldfile:
			trim = line.strip()
			if trim == '<!-- navbar start -->':
				skip = True
				line = addNavBar(file)
			elif trim == '<!-- docbar start -->':
				skip = True
				line = addDocBar(file)
			elif trim == '<!-- footer start -->':
				skip = True
				line = addFooter(file)
			elif trim == '<!-- navbar end -->' or trim == '<!-- docbar end -->' or trim == '<!-- footer end -->':
				skip = False
			elif skip:
				continue

			lines.append(line.format(root=root,docs=docs))

		oldfile.close()
		with open(folder + file, 'w') as newfile:
			newfile.writelines(lines)

for folder in ['', 'docs/']:
	for file in glob.glob(folder + '*.html'):
		print (file)
		processFile(folder, file[len(folder):])
