/*
 Source File : BasicModification.cpp


 Copyright 2012 Gal Kahana PDFWriter

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.


 */
#include "BasicModification.h"
#include "PDFWriter.h"
#include "PDFPage.h"
#include "PDFModifiedPage.h"
#include "PDFRectangle.h"
#include "PageContentContext.h"
#include "OutputStreamTraits.h"
#include "InputFile.h"
#include "OutputFile.h"

#include <iostream>

using namespace PDFHummus;
using namespace std;


void ModifyPage() {

    EStatusCode status = eSuccess;

    PDFWriter pdfWriter;
    pdfWriter.ModifyPDF("test.pdf", ePDFVersionMax, "test_modified.pdf");

    //AbstractContentContext* contentContext = modifiedPage.StartContentContext();
    //AbstractContentContext::TextOptions opt(pdfWriter.GetFontForFile("fonts/arial.ttf"),14,AbstractContentContext::eGray,0);
    //contentContext->WriteText(75,805,"Test Text",opt);

    //contentContext->DrawRectangle(0, 0, 200, 200);

    /*status = modifiedPage.EndContentContext();
    if(status != eSuccess)
    {
        cout<<"failed in EndContentContext\n";
        return;
    }*/

    for (int i = 0; i < 4; ++i) {
        PDFModifiedPage* modifiedPage = new PDFModifiedPage(&pdfWriter, i);
        status = modifiedPage->AttachURLLinktoCurrentPage("www.google.com", PDFRectangle(0, 0, 100, 100));
        status = modifiedPage->AttachURLLinktoCurrentPage("www.lenta.ru", PDFRectangle(200, 200, 400, 400));
        status = modifiedPage->WritePage();
        delete modifiedPage;

        /*PDFModifiedPage* modifiedPage2 = new PDFModifiedPage(&pdfWriter, i);
        status = modifiedPage2->AttachURLLinktoCurrentPage("www.google.com", PDFRectangle(200, 200, 100, 100));
        status = modifiedPage2->WritePage();
        delete modifiedPage2;*/
    }

    if(status != eSuccess)
    {
        cout<<"failed in WritePage\n";
        return;
    }

    status = pdfWriter.EndPDF();
    if(status != eSuccess)
    {
        cout<<"failed in end PDF\n";
        return;
    }
}

// Use this to repair modified pdf
// gs -o test_out.pdf -sDEVICE=pdfwrite -dPDFSETTINGS=/prepress test.pdf

bool TestBasicFileModification(string inSourceFileName)
{
	PDFWriter pdfWriter;
	EStatusCode status = eSuccess;
	do
	{
 		status = pdfWriter.ModifyPDF(inSourceFileName, ePDFVersionMax, string("Modified_") + inSourceFileName,
                                     LogConfiguration(true, true, string("Modified") + inSourceFileName + string(".log")));

		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed to start PDF\n";
			break;
		}

		PDFPage* page = new PDFPage();
		page->SetMediaBox(PDFRectangle(0,0,595,842));

		PageContentContext* contentContext = pdfWriter.StartPageContentContext(page);
		if(NULL == contentContext)
		{
			status = PDFHummus::eFailure;
			cout<<"failed to create content context for page\n";
			break;
		}

		PDFUsedFont* font = pdfWriter.GetFontForFile("couri.ttf");
		if(!font)
		{
			status = PDFHummus::eFailure;
			cout<<"Failed to create font object for couri.ttf\n";
			break;
		}


		// Draw some text
		contentContext->BT();
		contentContext->k(0,0,0,1);

		contentContext->Tf(font,1);

		contentContext->Tm(30,0,0,30,78.4252,662.8997);

		EStatusCode encodingStatus = contentContext->Tj("about");
		if(encodingStatus != PDFHummus::eSuccess)
			cout<<"Could not find some of the glyphs for this font";

		// continue even if failed...want to see how it looks like
		contentContext->ET();

		status = pdfWriter.EndPageContentContext(contentContext);
		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed to end page content context\n";
			break;
		}

		status = pdfWriter.WritePageAndRelease(page);
		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed to write page\n";
			break;
		}

		status = pdfWriter.EndPDF();
		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed in end PDF\n";
			break;
		}
	}while(false);

	return status == PDFHummus::eSuccess;
}

bool TestInPlaceFileModification(string inSourceFileName)
{
	PDFWriter pdfWriter;
	EStatusCode status = eSuccess;
	do
	{
        // first copy source file to target
        {
            InputFile sourceFile;

            status = sourceFile.OpenFile(inSourceFileName);

            if(status != eSuccess)
            {
                cout<<"failed to open source PDF\n";
                break;
            }

            OutputFile targetFile;

            status = targetFile.OpenFile(string("InPlaceModified_") + inSourceFileName);
            if(status != eSuccess)
            {
                cout<<"failed to open target PDF\n";
                break;
            }

            OutputStreamTraits traits(targetFile.GetOutputStream());
            status = traits.CopyToOutputStream(sourceFile.GetInputStream());
            if(status != eSuccess)
            {
                cout<<"failed to copy source to target PDF\n";
                break;
            }

            sourceFile.CloseFile();
            targetFile.CloseFile();

        }

        // now modify in place


 		status = pdfWriter.ModifyPDF(string("InPlaceModified_") + inSourceFileName, ePDFVersionMax, "",
                                     LogConfiguration(true, true, string("InPlaceModified_") + inSourceFileName + string(".log")));

		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed to start PDF\n";
			break;
		}

		PDFPage* page = new PDFPage();
		page->SetMediaBox(PDFRectangle(0,0,595,842));

		PageContentContext* contentContext = pdfWriter.StartPageContentContext(page);
		if(NULL == contentContext)
		{
			status = PDFHummus::eFailure;
			cout<<"failed to create content context for page\n";
			break;
		}

		PDFUsedFont* font = pdfWriter.GetFontForFile("couri.ttf");
		if(!font)
		{
			status = PDFHummus::eFailure;
			cout<<"Failed to create font object for couri.ttf\n";
			break;
		}


		// Draw some text
		contentContext->BT();
		contentContext->k(0,0,0,1);

		contentContext->Tf(font,1);

		contentContext->Tm(30,0,0,30,78.4252,662.8997);

		EStatusCode encodingStatus = contentContext->Tj("about");
		if(encodingStatus != PDFHummus::eSuccess)
			cout<<"Could not find some of the glyphs for this font";

		// continue even if failed...want to see how it looks like
		contentContext->ET();

		status = pdfWriter.EndPageContentContext(contentContext);
		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed to end page content context\n";
			break;
		}

		status = pdfWriter.WritePageAndRelease(page);
		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed to write page\n";
			break;
		}

		status = pdfWriter.EndPDF();
		if(status != PDFHummus::eSuccess)
		{
			cout<<"failed in end PDF\n";
			break;
		}
	}while(false);

	return status == PDFHummus::eSuccess;
}
