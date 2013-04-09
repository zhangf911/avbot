##
## Process this file with automake to produce Makefile.in
##

AM_CPPFLAGS = -Wall -pedantic -pipe -W -Wfloat-equal -Wcast-align -Wsign-compare -Wpointer-arith \
                -Wswitch -Wunknown-pragmas -Wconversion -Wundef -Wcast-qual \
                -Wwrite-strings -Wformat-security -Wmissing-format-attribute -Wno-long-long

noinst_LTLIBRARIES = libgloox.la

libgloox_la_SOURCES = jid.cpp parser.cpp clientbase.cpp tag.cpp stanza.cpp logsink.cpp \
                        prep.cpp base64.cpp client.cpp component.cpp \
                        disco.cpp adhoc.cpp privatexml.cpp registration.cpp \
                        nonsaslauth.cpp rosteritem.cpp rostermanager.cpp privacyitem.cpp \
                        annotations.cpp privacymanager.cpp lastactivity.cpp \
                        flexoff.cpp dataform.cpp dataformfield.cpp dataformfieldcontainer.cpp \
                        messagesession.cpp messageeventfilter.cpp chatstatefilter.cpp gloox.cpp \
                        inbandbytestream.cpp messagefilter.cpp vcard.cpp \
                        vcardmanager.cpp md5.cpp sha.cpp search.cpp dataformreported.cpp dataformitem.cpp \
                        mucroom.cpp mucmessagesession.cpp oob.cpp vcardupdate.cpp stanzaextensionfactory.cpp \
                        delayeddelivery.cpp gpgencrypted.cpp gpgsigned.cpp \
                        uniquemucroom.cpp instantmucroom.cpp compressionzlib.cpp \
			tlsschannel.cpp tlsdefault.cpp mutex.cpp amp.cpp \
                        pubsubitem.cpp pubsubmanager.cpp \
                        error.cpp util.cpp iq.cpp message.cpp presence.cpp \
                        subscription.cpp capabilities.cpp chatstate.cpp connectionbosh.cpp\
                        messageevent.cpp receipt.cpp nickname.cpp eventdispatcher.cpp \
                        pubsubevent.cpp xhtmlim.cpp featureneg.cpp \
                        shim.cpp softwareversion.cpp attention.cpp \
                        tlsopensslclient.cpp tlsopensslbase.cpp compressiondefault.cpp 

libgloox_la_LDFLAGS = -static
libgloox_la_LIBADD = ${LIBS}
libgloox_la_CFLAGS = $(CPPFLAGS)

noinst_HEADERS = adhoccommandprovider.h      privacymanager.h        tag.h \
                            adhoc.h attention.h iqhandler.h             privatexml.h \
                            annotations.h             client.h                privatexmlhandler.h \
                            annotationshandler.h      component.h             registration.h \
                            bookmarkhandler.h         registrationhandler.h   jid.h         messagehandler.h        rosteritem.h \
                            clientbase.h              rosterlistener.h        taghandler.h \
                            connectionlistener.h      rostermanager.h         gloox.h \
                            glooxversion.h \
                            disco.h                   presencehandler.h       subscriptionhandler.h \
                            discohandler.h            privacyitem.h           stanza.h \
                            disconodehandler.h        privacylisthandler.h    loghandler.h \
                            lastactivity.h            lastactivityhandler.h   flexoff.h \
                            flexoffhandler.h          dataform.h              dataformfield.h \
                            dataformitem.h            dataformfieldcontainer.h dataformreported.h \
                            macros.h                  logsink.h               messagesession.h \
                            messageeventhandler.h     messagesessionhandler.h chatstatehandler.h \
                            chatstatefilter.h         messageeventfilter.h    inbandbytestream.h \
                            messagefilter.h           vcard.h \
                            vcardmanager.h            vcardhandler.h          adhochandler.h \
                            search.h                  searchhandler.h         statisticshandler.h \
                            resource.h                mucroom.h               mucroomhandler.h \
                            mucroomconfighandler.h    parser.h                \
                            mucinvitationhandler.h    stanzaextension.h       oob.h \
                            vcardupdate.h             delayeddelivery.h       base64.h \
                            gpgencrypted.h            gpgsigned.h \
                            md5.h                     sha.h                   uniquemucroom.h \
                            instantmucroom.h          tlsbase.h               tlshandler.h \
                            connectionbase.h          connectiondatahandler.h compressiondatahandler.h \
                            compressionbase.h         \
                            tlsdefault.h              simanager.h sihandler.h      event.h                     \
                            softwareversion.h \
                            eventhandler.h \
                            mutex.h                   mutexguard.h             \
                            amp.h                   pubsubmanager.h \
                            iq.h                      pubsub.h                \
                            pubsubresulthandler.h     error.h                 chatstate.h \
                            connectionhandler.h \
                            message.h                 presence.h              subscription.h \
                            bytestream.h              bytestreamdatahandler.h bytestreamhandler.h \
                            capabilities.h            connectionbosh.h        featureneg.h \
                                      messageevent.h          receipt.h \
                            nickname.h                pubsubevent.h           xhtmlim.h \
                            eventdispatcher.h         \
                            pubsubitem.h shim.h util.h \
                            compressiondefault.h

noinst_HEADERS += config.h prep.h nonsaslauth.h mucmessagesession.h stanzaextensionfactory.h \
                   tlsopensslbase.h tlsschannel.h \
                   compressionzlib.h rosteritemdata.h tlsopensslclient.h 

EXTRA_DIST = version.rc
