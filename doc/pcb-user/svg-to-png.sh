DPI=57
convert -density $DPI front.svg -background white -alpha remove front-bg-white.png
convert -density $DPI front.svg front-bg-alpha.png
