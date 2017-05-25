# Project Name

pdfParser

## Installation

1) Download and build [PDFWriter project](https://github.com/galkahana/PDF-Writer).
Build instructions [here](https://github.com/galkahana/PDF-Writer/wiki/Building-and-running-samples).

2) Download and extract [TCLAP project](http://sourceforge.net/project/showfiles.php?group_id=76645)

3) Download pdfParser.

Project directory structure should look like this:

    SomeFolder
	    |--pdfParser
	    |
	    |--PDFWriter
	    |
	    |--tclap-1.2.1

4) Go to pdfParser directory and run this commands:

    cmake -G "Unix Makefiles"
    make

pdfParser executable will be generated in 'build' folder.

5) Or just open pdfParser.cbp in Code::Blocks.


## Usage

To extract all text

    ./pdfParser -c extract -p path/to/file.pdf -o out/put/dir

To insert links

    ./pdfParser -c insert -p path/to/file.pdf -o out/put/dir -r "((?:\d\.)?\d{3}\.\d{3})" -l 18 -u http://www.lyreco.com/webshop/DADA/product-product-{placeholder}.html

To verify PDF

    ./pdfParser -c verify -p path/to/file.pdf

To show detailed info about command line arguments

    ./pdfParser --help


Return value
    
    0 - app exited normally
    1 - output directory not exists
    2 - input file not found
    3 - input arguments parsing error
    4 - PDFWriter failed to parse pdf
    5 - PDF is protected by password


