with open('xpalettevals', 'r') as palettefile:
    content = palettefile.readlines()

xcolors = [x.strip() for x in content]
yoff = -25
for i in range(0, 256):
    if i % 16 == 0:
        yoff += 25
    print(f"""<text
       id=\"text{i}\"
       y=\"{yoff}\"
       x=\"{(i%16)*25}\"
       style=\"font-size:10px;line-height:1.25;font-family:'Linux Libertine Display O';-inkscape-font-specification:'Linux Libertine Display O';fill:{xcolors[i]};fill-opacity:1\"
       xml:space=\"preserve\"><tspan
         style=\"stroke-width:0.26458332\"
         y=\"{yoff}\"
         x=\"{(i%16)*25}\"
         id=\"tspan21\">{i}</tspan></text>""")
