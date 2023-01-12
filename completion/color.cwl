# color.sty
# tbraun 11/2/2006
# updated 2022/06/05 for v1.3d

#keyvals:\usepackage/color#c
monochrome
debugshow
setpagesize
nosetpagesize
dvips
xdvi
dvipdf
dvipdfm
dvipdfmx
pdftex
luatex
dvisvgm
xetex
dvipsone
dviwindo
emtex
dviwin
oztex
textures
pctexps
pctexwin
pctexhp
pctex32
truetex
tcidvi
vtex
dvipsnames
nodvipsnames
usenames
#endkeyvals

\textcolor{color}{text}
\textcolor[model]{color}{text}
\mathcolor{color}{math}
\mathcolor[model]{color}{math}
\pagecolor{color}
\nopagecolor
\definecolor{name%specialDef}{model}{color-spec}#s#%color
\DefineNamedColor{type}{name}{model}{spec}#*
\normalcolor
\color[model]{color}
\color{color}
\colorbox[model]{color}{text}
\colorbox{color}{text}
\fcolorbox[model]{border-color}{fill-color}{text}
\fcolorbox{border-color}{fill-color}{text}
# repeat with #S for color completion
\fcolorbox[model]{color}{color}{text}#S
\fcolorbox{color}{color}{text}#S

# colors
red#B
blue#B
green#B
yellow#B
black#B
white#B
cyan#B
magenta#B

## dvipsnames colors
#ifOption:dvipsnames
Apricot#B
Aquamarine#B
Bittersweet#B
Black#B
Blue#B
BlueGreen#B
BlueViolet#B
BrickRed#B
Brown#B
BurntOrange#B
CadetBlue#B
CarnationPink#B
Cerulean#B
CornflowerBlue#B
Cyan#B
Dandelion#B
DarkOrchid#B
Emerald#B
ForestGreen#B
Fuchsia#B
Goldenrod#B
Gray#B
Green#B
GreenYellow#B
JungleGreen#B
Lavender#B
LimeGreen#B
Magenta#B
Mahogany#B
Maroon#B
Melon#B
MidnightBlue#B
Mulberry#B
NavyBlue#B
OliveGreen#B
Orange#B
OrangeRed#B
Orchid#B
Peach#B
Periwinkle#B
PineGreen#B
Plum#B
ProcessBlue#B
Purple#B
RawSienna#B
Red#B
RedOrange#B
RedViolet#B
Rhodamine#B
RoyalBlue#B
RoyalPurple#B
RubineRed#B
Salmon#B
SeaGreen#B
Sepia#B
SkyBlue#B
SpringGreen#B
Tan#B
TealBlue#B
Thistle#B
Turquoise#B
Violet#B
VioletRed#B
White#B
WildStrawberry#B
Yellow#B
YellowGreen#B
YellowOrange#B
#endif
