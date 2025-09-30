#
#  Makefile for the Errno utility used for Watt-32 developement.
#
#  NOTE: Some make programs may not be able to generate all
#        targets due to DPMI/DOS-extender conflicts etc.
#        Use your native make tool to make only the target
#        you need.
#          E.g. say: 'wmake -f errnos.mak wc_err.exe' for Watcom
#                or: 'maker -f errnos.mak hc_err.exe' for High-C
#

all: bcc_err.exe tcc_err.exe wc_err.exe ms_err.exe ms32_err.exe hc_err.exe dj_err.exe lcc_err.exe

bcc_err.exe: errnos.c
	bcc -I..\inc -ml -ebcc_err.exe errnos.c

tcc_err.exe: errnos.c
	tcc -I..\inc -ml -etcc_err.exe errnos.c

wc_err.exe: errnos.c
	wcl -I..\inc -ml -zq -fe=wc_err.exe -fr=nul errnos.c

hc_err.exe: errnos.c
	hc386 -I..\inc -Hldopt=-nomap -o hc_err.exe errnos.c

dj_err.exe: errnos.c
	gcc -s -I../inc -o dj_err.exe errnos.c

dm_err.exe: errnos.c
	dmc -ml -g -I..\inc -odm_err.exe errnos.c

ms_err.exe: errnos.c
	cl -I..\inc -AL -Fems_err.exe errnos.c

vc_err.exe: errnos.c
	cl -I..\inc -Fevc_err.exe errnos.c

mw_err.exe: errnos.c
	gcc -s -I../inc -o mw_err.exe errnos.c

ls_err.exe: errnos.c
	cl386 /E0 /I..\inc /e=ls_err.exe errnos.c
     #  cl386 /L$(LADSOFT)\lib /E0 /I..\inc /e=ls_err.exe -$$D=DOS32A errnos.c

lcc_err.exe: errnos.c
	lcc -I..\inc -A errnos.c
	lcclnk errnos.obj -o lcc_err.exe

po_err.exe: errnos.c
	pocc -Ze -c -I$(PELLESC)\include -I$(PELLESC)\include\win -I..\inc $**
	polink -out:$@ -libpath:$(PELLESC)\lib errnos.obj

clean:
	@del bcc_err.exe wc_err.exe ms_err.exe vc_err.exe hc_err.exe \
             tcc_err.exe dj_err.exe mw_err.exe ls_err.exe lcc.exe po_err.exe \
             errnos.obj

