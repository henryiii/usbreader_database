#include "TFile.h"
#include "TTree.h"

void RunThroughData() {

    TFile *f = TFile::Open("recodata1.root");
    if(f==0) {
        printf("Failed to find file");
        return;
    }


    TTreeReader xzReader ("Event Tree XZ", f);
    TTreeReader yzReader ("Event Tree YZ", f);

    TTreeReaderValue<Double_t> x_inter(xzReader, "Yinter");
    TTreeReaderValue<Double_t> y_inter(yzReader, "Yinter");
    TTreeReaderValue<Double_t> x_slope(xzReader, "Slope");
    TTreeReaderValue<Double_t> y_slope(yzReader, "Slope");

    TH2F *proj_hist = new TH2F("prog_hist", "Projection above the detector", 25,-100,100,25,-100,100);

    printf("x_inter, x_slope, y_inter, y_slope\n");
    while(xzReader.Next() && yzReader.Next()) {
        printf("%f, %f, %f, %f\n", *x_inter, *x_slope, *y_inter, *y_slope);
        double height = 400;
        double x = *x_inter + height*(*x_slope);
        double y = *y_inter + height*(*y_slope);
        proj_hist->Fill(x,y);
    }
    proj_hist->Draw("COLZ");
    gStyle->SetPalette(1,0);
}
