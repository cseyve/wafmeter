#include <QtCore/QCoreApplication>

#include <QDir>
#include <iostream>

#include "imgutils.h"
#include "wafmeter.h"
void addColumn(FILE * f, FILE * fhtml,
	float val) {
	char string[32];
	sprintf(string, "%g\t", val);

	while(strstr(string, ".")) {
		*(strstr(string, ".")) = ',';
	}
	fprintf(f, "%s", string);
	fprintf(fhtml, "<td>%g</td> ", val);

}
int main(int argc, char *argv[])
{
	if(argc < 0) {
		fprintf(stderr, "Usage: ./wafconsole <path to pictures files>\n");
	}
    QCoreApplication a(argc, argv);

	// List files
//	QDir::QDir ( const QString & path, const QString & nameFilter, SortFlags sort = SortFlags( Name | IgnoreCase ), Filters filters = AllEntries )
	QDir dir(argv[1], "*.jpg *.png");

	WAFMeter wafMeter;
	IplImage * img = NULL;
	QFileInfoList list = dir.entryInfoList();
	std::cout << "     Bytes Filename" << std::endl;
	FILE * f = fopen("measures.txt", "w");
	FILE * fhtml = fopen("measures.html", "w");
	if(f) {
		fprintf(f, "File\tNbContour\tNbSegments\tScalar\tNbCorners\tNbButtons\n");
	}
	if(fhtml) {
		fprintf(fhtml,
				//"File\tNbContour\tNbSegments\tScalar\tNbCorners\tNbButtons\n"
				"<html><head><title \"WAF Measures for directory %s\">"
				"</head><body>"
				"<table cellspacing=1 cellpadding=1 border=1>\n"
				"<tr>\n"
					"\t<td>File</td>\n"
					"\t<td>µ WAF</td>\n"
					"\t<td>Nb contour</td>\n"
					"\t<td>µ contours</td>\n"
					"\t<td>Nb segments</td>\n"
					"\t<td>µ segments</td>\n"
					"\t<td>Scalar</td>\n"
					"\t<td>µ scalar</td>\n"
					"\t<td>NbCorners</td>\n"
					"\t<td>µ corners</td>\n"
					"\t<td>NbButtons</td>\n"
					"\t<td>µ buttons</td>\n"
					"</tr>\n\n", argv[1]
				);
	}

	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		std::cout << qPrintable(QString("%1 %2").arg(fileInfo.size(), 10)
								.arg(fileInfo.fileName()));
		std::cout << std::endl;

		img = tmLoadImage(dir.filePath(fileInfo.fileName()));
		wafMeter.setImage(img);
		tmReleaseImage(&img);
		t_waf_info_details details = wafMeter.getWAFDetails();
		if(f) {
			//fprintf(f, "File\tNbContour\tNbSegments\tScalar\tNbCorners\tNbButtons\n");
			fprintf(f, "%s\t", dir.filePath(fileInfo.fileName()).ascii());
			fprintf(fhtml, "<tr><td>"
					"%s<br>"
					"<img src=\"%s\" height=\"100\">"
					"</td>",
					dir.filePath(fileInfo.fileName()).ascii(),
					dir.filePath(fileInfo.fileName()).ascii()
					);

			// append values
			addColumn(f,fhtml, details.mu_color);
			addColumn(f,fhtml, details.nb_contours);
			addColumn(f,fhtml, details.mu_contours);
			addColumn(f,fhtml, details.nb_segments);
			addColumn(f,fhtml, details.mu_segments);
			addColumn(f,fhtml, details.scalar_mean);
			addColumn(f,fhtml, details.mu_scalar);
			addColumn(f,fhtml, details.nb_corners);
			addColumn(f,fhtml, details.mu_corners);
			addColumn(f,fhtml, details.nb_buttons);
			addColumn(f,fhtml, details.mu_buttons);



			fprintf(f, "\n");
			fprintf(fhtml, "</tr>\n");
		}
	}
	if(f) {
		fclose(f);
	}
	if(fhtml) {
		fprintf(fhtml, "\n</table>\n</body>\n</html>\n");
		fclose(fhtml);
	}

	return 0;
    return a.exec();
}
