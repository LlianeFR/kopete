/*
    videodevicepool.cpp  -  Kopete Video Device Low-level Support

    Copyright (c) 2005-2006 by Cláudio da Silveira Pinheiro   <taupter@gmail.com>
    Copyright (c) 2010      by Frank Schaefer                 <fschaefer.oss@googlemail.com>

    Kopete    (c) 2002-2003      by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#define ENABLE_AV

#include <assert.h>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <qdir.h>
#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/deviceinterface.h>
#include <solid/video.h>


#include "videodevice.h"
#include "videodevicepool.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

namespace Kopete {

namespace AV {

VideoDevicePool *VideoDevicePool::s_self = NULL;
__u64 VideoDevicePool::m_clients = 0;

VideoDevicePool* VideoDevicePool::self()
{
//	kDebug() << "libkopete (avdevice): self() called";
	if (s_self == NULL)
	{
		s_self = new VideoDevicePool;
		if (s_self)
			m_clients = 0;
	}
//	kDebug() << "libkopete (avdevice): self() exited successfuly";
	return s_self;
}

VideoDevicePool::VideoDevicePool()
: m_current_device(0)
{
	connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), SLOT(deviceAdded(const QString &)) );
	connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString&)), SLOT(deviceRemoved(const QString &)) );
}


VideoDevicePool::~VideoDevicePool()
{
}




/*!
    \fn VideoDevicePool::open(int device)
 */
int VideoDevicePool::open(int device)
{
    /// @todo implement me
	kDebug() << "called with device" << device;
	m_ready.lock();
	if (!m_videodevice.size())
	{
		kDebug() << "open(): No devices found. Must scan for available devices." << m_current_device;
		scanDevices();
	}
	if (!m_videodevice.size() || (device >= m_videodevice.size()))
	{
		kDebug() << "open(): Device not found. bailing out." << m_current_device;
		m_ready.unlock();
		return EXIT_FAILURE;
	}
	int current_device = m_current_device;
	if (device < 0)
	{
		kDebug() << "Trying to load saved device (using default device if not available)";
		loadSelectedDevice();	// Set m_current_device to saved device (if device available)
	}
	else
		m_current_device = device;
	int isopen = EXIT_FAILURE;
	if ((m_current_device != current_device) || !isOpen())
	{
		m_clients = 0;
		m_videodevice[current_device].close();
		isopen = m_videodevice[m_current_device].open();
		if (isopen == EXIT_SUCCESS)
		{
			loadDeviceConfig(); // Load and apply device parameters
			m_clients++;
		}
	}
	else
	{
		isopen = EXIT_SUCCESS;
		m_clients++;
	}
	kDebug() << "Number of clients: " << m_clients;
	m_ready.unlock();
	return isopen;
}

bool VideoDevicePool::isOpen()
{
	return m_videodevice[currentDevice()].isOpen();
}

/*!
    \fn VideoDevicePool::showDeviceCapabilities(int device)
 */
int VideoDevicePool::showDeviceCapabilities(unsigned int device)
{
	return m_videodevice[device].showDeviceCapabilities();
}

int VideoDevicePool::width()
{
	return m_videodevice[currentDevice()].width();
}

int VideoDevicePool::minWidth()
{
	return m_videodevice[currentDevice()].minWidth();
}

int VideoDevicePool::maxWidth()
{
	return m_videodevice[currentDevice()].maxWidth();
}

int VideoDevicePool::height()
{
	return m_videodevice[currentDevice()].height();
}

int VideoDevicePool::minHeight()
{
	return m_videodevice[currentDevice()].minHeight();
}

int VideoDevicePool::maxHeight()
{
	return m_videodevice[currentDevice()].maxHeight();
}

int VideoDevicePool::setSize( int newwidth, int newheight)
{
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].setSize(newwidth, newheight);
	else
	{
		kDebug() << "VideoDevicePool::setSize() fallback for no device.";
		m_buffer.width=newwidth;
		m_buffer.height=newheight;
		m_buffer.pixelformat=	PIXELFORMAT_RGB24;
		m_buffer.data.resize(m_buffer.width*m_buffer.height*3);
		kDebug() << "VideoDevicePool::setSize() buffer size: "<< m_buffer.data.size();
	}
	return EXIT_SUCCESS;
}

/*!
    \fn VideoDevicePool::close()
 */
int VideoDevicePool::close()
{
    /// @todo implement me
	if(m_clients)
		m_clients--;
	if((currentDevice() < m_videodevice.size())&&(!m_clients))
		return m_videodevice[currentDevice()].close();
	if(m_clients)
		kDebug() << "VideoDevicePool::close() The video device is still in use.";
	if(currentDevice() >= m_videodevice.size())
		kDebug() << "VideoDevicePool::close() Current device out of range.";
	return EXIT_FAILURE;
}

/*!
    \fn VideoDevicePool::startCapturing()
 */
int VideoDevicePool::startCapturing()
{
	kDebug() << "startCapturing() called.";
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].startCapturing();
	return EXIT_FAILURE;
}


/*!
    \fn VideoDevicePool::stopCapturing()
 */
int VideoDevicePool::stopCapturing()
{
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].stopCapturing();
	return EXIT_FAILURE;
}


// Implementation of the methods that get / set input's adjustment parameters

/*!
    \fn QList<NumericVideoControl> VideoDevicePool::getSupportedNumericControls()
    \return A list of all supported numeric controls for the current input
    \brief Returns the supported numeric controls for the current input
 */
QList<NumericVideoControl> VideoDevicePool::getSupportedNumericControls()
{
	if (currentDevice() < m_videodevice.size() )
		return m_videodevice[currentDevice()].getSupportedNumericControls();
	else
		return QList<NumericVideoControl>();
}

/*!
    \fn QList<BooleanVideoControl> VideoDevicePool::getSupportedBooleanControls()
    \return A list of all supported boolean controls for the current input
    \brief Returns the supported boolean controls for the current input
 */
QList<BooleanVideoControl> VideoDevicePool::getSupportedBooleanControls()
{
	if (currentDevice() < m_videodevice.size() )
		return m_videodevice[currentDevice()].getSupportedBooleanControls();
	else
		return QList<BooleanVideoControl>();
}

/*!
    \fn QList<MenuVideoControl> VideoDevicePool::getSupportedMenuControls()
    \return A list of all supported menu-controls for the current input
    \brief Returns the supported menu-controls for the current input
 */
QList<MenuVideoControl> VideoDevicePool::getSupportedMenuControls()
{
	if (currentDevice() < m_videodevice.size() )
		return m_videodevice[currentDevice()].getSupportedMenuControls();
	else
		return QList<MenuVideoControl>();
}

/*!
    \fn QList<ActionVideoControl> VideoDevicePool::getSupportedActionControls()
    \return A list of all supported action-controls for the current input
    \brief Returns the supported action-controls for the current input
 */
QList<ActionVideoControl> VideoDevicePool::getSupportedActionControls()
{
	if (currentDevice() < m_videodevice.size() )
		return m_videodevice[currentDevice()].getSupportedActionControls();
	else
		return QList<ActionVideoControl>();
}

/*!
    \fn int VideoDevicePool::getControlValue(quint32 ctrl_id, qint32 * value)
    \param ctrl_id ID of the video-control
    \param value Pointer to the variable, which recieves the value of the querried video-control.
                 For boolean controls, the value is 0 or 1.
                 For menu-controls, the value is the index of the currently selected option.
    \return The result-code, currently EXIT_SUCCESS or EXIT_FAILURE
    \brief Reads the value of a video-control
 */
int VideoDevicePool::getControlValue(quint32 ctrl_id, qint32 * value)
{
	if (currentDevice() < m_videodevice.size() )
		return m_videodevice[currentDevice()].getControlValue(ctrl_id, value);
	else
		return EXIT_FAILURE;
}

/*!
    \fn int VideoDevicePool::setControlValue(quint32 ctrl_id, qint32 value)
    \param ctrl_id ID of the video-control
    \param value The value that should be set.
                 For boolean controls, the value must be 0 or 1.
                 For menu-controls, the value must be the index of the option.
                 For action-controls, the value is ignored.
    \return The result-code, currently EXIT_SUCCESS or EXIT_FAILURE
    \brief Sets the value of a video-control
 */
int VideoDevicePool::setControlValue(quint32 ctrl_id, qint32 value)
{
	if (currentDevice() < m_videodevice.size() )
		return m_videodevice[currentDevice()].setControlValue(ctrl_id, value);
	else
		return EXIT_FAILURE;
}


/*!
    \fn VideoDevicePool::getFrame()
 */
int VideoDevicePool::getFrame()
{
//	kDebug() << "VideoDevicePool::getFrame() called.";
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].getFrame();
	else
	{
		kDebug() << "VideoDevicePool::getFrame() fallback for no device.";
		for(int loop=0; loop < m_buffer.data.size(); loop+=3)
		{
			m_buffer.data[loop]   = 255;
			m_buffer.data[loop+1] = 0;
			m_buffer.data[loop+2] = 0;
		}
	}
	kDebug() << "VideoDevicePool::getFrame() exited successfuly.";

	return EXIT_SUCCESS;
}

/*!
    \fn VideoDevicePool::getQImage(QImage *qimage)
 */
int VideoDevicePool::getImage(QImage *qimage)
{
//	kDebug() << "VideoDevicePool::getImage() called.";
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].getImage(qimage);
	else
	{
		kDebug() << "VideoDevicePool::getImage() fallback for no device.";

		// do NOT delete qimage here, as it is received as a parameter
		if (qimage->width() != width() || qimage->height() != height())
			*qimage = QImage(width(), height(), QImage::Format_RGB32);

		uchar *bits=qimage->bits();
		switch(m_buffer.pixelformat)
		{
			case PIXELFORMAT_NONE	: break;
			case PIXELFORMAT_GREY	: break;
			case PIXELFORMAT_RGB332	: break;
			case PIXELFORMAT_RGB555	: break;
			case PIXELFORMAT_RGB555X: break;
			case PIXELFORMAT_RGB565	: break;
			case PIXELFORMAT_RGB565X: break;
			case PIXELFORMAT_RGB24	:
				{
					kDebug() << "VideoDevicePool::getImage() fallback for no device - RGB24.";
					int step=0;
					for(int loop=0;loop < qimage->numBytes();loop+=4)
					{
						bits[loop]   = m_buffer.data[step];
						bits[loop+1] = m_buffer.data[step+1];
						bits[loop+2] = m_buffer.data[step+2];
						bits[loop+3] = 255;
						step+=3;
					}
				}
				break;
			case PIXELFORMAT_BGR24	: break;
				{
					int step=0;
					for(int loop=0;loop < qimage->numBytes();loop+=4)
					{
						bits[loop]   = m_buffer.data[step+2];
						bits[loop+1] = m_buffer.data[step+1];
						bits[loop+2] = m_buffer.data[step];
						bits[loop+3] = 255;
						step+=3;
					}
				}
				break;
			case PIXELFORMAT_RGB32	: memcpy(bits,&m_buffer.data[0], m_buffer.data.size());
				break;
			case PIXELFORMAT_BGR32	: break;
			case PIXELFORMAT_YUYV   : break;
			case PIXELFORMAT_UYVY   : break;
			case PIXELFORMAT_YUV420P: break;
			case PIXELFORMAT_YUV422P: break;
			default: break;
		}
	}
	kDebug() << "VideoDevicePool::getImage() exited successfuly.";
	return EXIT_SUCCESS;
}

/*!
    \fn Kopete::AV::VideoDevicePool::selectInput(int input)
 */
int VideoDevicePool::selectInput(int newinput)
{
	kDebug() << "VideoDevicePool::selectInput(" << newinput << ") called.";
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].selectInput(newinput);
	else
		return 0;
}

/*!
    \fn Kopete::AV::VideoDevicePool::fillDeviceKComboBox(KComboBox *combobox)
 */
int VideoDevicePool::fillDeviceKComboBox(KComboBox *combobox)
{
    /// @todo implement me
	kDebug() << "Called.";
// check if KComboBox is a valid pointer.
	if (combobox != NULL)
	{
		combobox->clear();
		kDebug() << "Combobox cleaned.";
		if(m_videodevice.size())
		{
			for (int loop=0; loop < m_videodevice.size(); loop++)
			{
				combobox->addItem(m_videodevice[loop].m_name);
				kDebug() << "Added device " << loop << ": " << m_videodevice[loop].m_name;
			}
			combobox->setCurrentIndex(currentDevice());
			combobox->setEnabled(true);
			return EXIT_SUCCESS;
		}
		combobox->setEnabled(false);
	}
	return EXIT_FAILURE;
}

/*!
    \fn Kopete::AV::VideoDevicePool::fillInputKComboBox(KComboBox *combobox)
 */
int VideoDevicePool::fillInputKComboBox(KComboBox *combobox)
{
    /// @todo implement me
	kDebug() << "Called.";
	if (combobox != NULL)
	{
		combobox->clear();
		if ( !m_videodevice.isEmpty() && (currentDevice()>=0) && currentDevice() < m_videodevice.size() )
		{
			if(m_videodevice[currentDevice()].inputs()>0)
			{
				for (int loop=0; loop < m_videodevice[currentDevice()].inputs(); loop++)
				{
					combobox->addItem(m_videodevice[currentDevice()].m_input[loop].name);
					kDebug() << "Added input " << loop << ": " << m_videodevice[currentDevice()].m_input[loop].name << " (tuner: " << m_videodevice[currentDevice()].m_input[loop].hastuner << ")";
				}
				combobox->setCurrentIndex(currentInput());
				combobox->setEnabled(true);
				return EXIT_SUCCESS;
			}
		}
		combobox->setEnabled(false);
	}
	return EXIT_FAILURE;
}

/*!
    \fn Kopete::AV::VideoDevicePool::fillStandardKComboBox(KComboBox *combobox)
 */
int VideoDevicePool::fillStandardKComboBox(KComboBox *combobox)
{
    /// @todo implement me
	kDebug() << "Called.";
	if (combobox != NULL)
	{
		combobox->clear();
		if ( !m_videodevice.isEmpty() && currentDevice() < m_videodevice.size() )
		{
			if(m_videodevice[currentDevice()].inputs()>0)
			{
				for (unsigned int loop=0; loop < 25; loop++)
				{
					if ( (m_videodevice[currentDevice()].m_input[currentInput()].m_standards) & (1 << loop) )
						combobox->addItem(m_videodevice[currentDevice()].signalStandardName( 1 << loop));
/*
				case STANDARD_PAL_B1	: return V4L2_STD_PAL_B1;	break;
				case STANDARD_PAL_G	: return V4L2_STD_PAL_G;	break;
				case STANDARD_PAL_H	: return V4L2_STD_PAL_H;	break;
				case STANDARD_PAL_I	: return V4L2_STD_PAL_I;	break;
				case STANDARD_PAL_D	: return V4L2_STD_PAL_D;	break;
				case STANDARD_PAL_D1	: return V4L2_STD_PAL_D1;	break;
				case STANDARD_PAL_K	: return V4L2_STD_PAL_K;	break;
				case STANDARD_PAL_M	: return V4L2_STD_PAL_M;	break;
				case STANDARD_PAL_N	: return V4L2_STD_PAL_N;	break;
				case STANDARD_PAL_Nc	: return V4L2_STD_PAL_Nc;	break;
				case STANDARD_PAL_60	: return V4L2_STD_PAL_60;	break;
				case STANDARD_NTSC_M	: return V4L2_STD_NTSC_M;	break;
				case STANDARD_NTSC_M_JP	: return V4L2_STD_NTSC_M_JP;	break;
				case STANDARD_NTSC_443	: return V4L2_STD_NTSC;		break; // Using workaround value because my videodev2.h header seems to not include this standard in struct __u64 v4l2_std_id
				case STANDARD_SECAM_B	: return V4L2_STD_SECAM_B;	break;
				case STANDARD_SECAM_D	: return V4L2_STD_SECAM_D;	break;
				case STANDARD_SECAM_G	: return V4L2_STD_SECAM_G;	break;
				case STANDARD_SECAM_H	: return V4L2_STD_SECAM_H;	break;
				case STANDARD_SECAM_K	: return V4L2_STD_SECAM_K;	break;
				case STANDARD_SECAM_K1	: return V4L2_STD_SECAM_K1;	break;
				case STANDARD_SECAM_L	: return V4L2_STD_SECAM_L;	break;
				case STANDARD_SECAM_LC	: return V4L2_STD_SECAM;	break; // Using workaround value because my videodev2.h header seems to not include this standard in struct __u64 v4l2_std_id
				case STANDARD_ATSC_8_VSB	: return V4L2_STD_ATSC_8_VSB;	break; // ATSC/HDTV Standard officially not supported by V4L2 but exists in videodev2.h
				case STANDARD_ATSC_16_VSB	: return V4L2_STD_ATSC_16_VSB;	break; // ATSC/HDTV Standard officially not supported by V4L2 but exists in videodev2.h
				case STANDARD_PAL_BG	: return V4L2_STD_PAL_BG;	break;
				case STANDARD_PAL_DK	: return V4L2_STD_PAL_DK;	break;
				case STANDARD_PAL	: return V4L2_STD_PAL;		break;
				case STANDARD_NTSC	: return V4L2_STD_NTSC;		break;
				case STANDARD_SECAM_DK	: return V4L2_STD_SECAM_DK;	break;
				case STANDARD_SECAM	: return V4L2_STD_SECAM;	break;
				case STANDARD_525_60	: return V4L2_STD_525_60;	break;
				case STANDARD_625_50	: return V4L2_STD_625_50;	break;
				case STANDARD_ALL	: return V4L2_STD_ALL;		break;

				combobox->insertItem(m_videodevice[currentDevice()].m_input[loop].name);
				kDebug() << "StandardKCombobox: Added input " << loop << ": " << m_videodevice[currentDevice()].m_input[loop].name << " (tuner: " << m_videodevice[currentDevice()].m_input[loop].hastuner << ")";*/
				}
				combobox->setCurrentIndex(currentInput());
				combobox->setEnabled(combobox->count());
				return EXIT_SUCCESS;
			}
		}
		combobox->setEnabled(false);
	}
	return EXIT_FAILURE;
}

/*!
    \fn Kopete::AV::VideoDevicePool::scanDevices()
 */
int VideoDevicePool::scanDevices()
{
    /// @todo implement me

	if (m_videodevice.isEmpty()) {
		kDebug() << "called";
#if defined(__linux__) && defined(ENABLE_AV)
		foreach (Solid::Device device,
				Solid::Device::listFromType(Solid::DeviceInterface::Video, QString())) {
			registerDevice( device );
		}

#endif
		kDebug() << "exited successfuly";
	} else {
		kDebug() << "Not scanning: initial device list already loaded";
	}
	return EXIT_SUCCESS;
}

void VideoDevicePool::registerDevice( Solid::Device & device )
{
	kDebug() << "New video device at " << device.udi();
	const Solid::Device * vendorDevice = &device;
	while ( vendorDevice->isValid() && vendorDevice->vendor().isEmpty() )
	{
		vendorDevice = new Solid::Device( vendorDevice->parentUdi() );
	}
	if ( vendorDevice->isValid() )
	{
		kDebug() << "vendor: " << vendorDevice->vendor() << ", product: " << vendorDevice->product();
	}
	Solid::Video * solidVideoDevice = device.as<Solid::Video>();
	if ( solidVideoDevice ) {
		QStringList protocols = solidVideoDevice->supportedProtocols();
		if ( protocols.contains( "video4linux" ) )
		{
			QStringList drivers = solidVideoDevice->supportedDrivers( "video4linux" );
			if ( drivers.contains( "video4linux" ) )
			{
				kDebug() << "V4L device path is" << solidVideoDevice->driverHandle( "video4linux" ).toString();
				VideoDevice videodevice;
				videodevice.setUdi( device.udi() );
				videodevice.setFileName(solidVideoDevice->driverHandle( "video4linux" ).toString());
				kDebug() << "Found device " << videodevice.full_filename;
				videodevice.open();
				if(videodevice.isOpen())
				{
					kDebug() << "File " << videodevice.full_filename << " was opened successfuly";
					videodevice.close();
					videodevice.m_modelindex=m_modelvector.addModel (videodevice.m_model); // Adds device to the device list and sets model number
					m_videodevice.push_back(videodevice);
				}
			}
		}
	}
}

/*!
    \fn Kopete::AV::VideoDevicePool::hasDevices()
 */
bool VideoDevicePool::hasDevices()
{
    /// @todo implement me
	if(m_videodevice.size())
		return true;
	return false;
}

/*!
    \fn Kopete::AV::VideoDevicePool::size()
 */
size_t VideoDevicePool::size()
{
    /// @todo implement me
	return m_videodevice.size();
}

/*!
    \fn Kopete::AV::VideoDevicePool::currentDevice()
 */
int VideoDevicePool::currentDevice()
{
    /// @todo implement me
	return m_current_device;
}

/*!
    \fn Kopete::AV::VideoDevicePool::currentDeviceUdi()
 */
QString VideoDevicePool::currentDeviceUdi()
{
	if (m_videodevice.size() && (m_current_device >= 0))
		return m_videodevice[m_current_device].udi();
	else
		return QString();
}

/*!
    \fn Kopete::AV::VideoDevicePool::currentInput()
 */
int VideoDevicePool::currentInput()
{
    /// @todo implement me
	return m_videodevice[currentDevice()].currentInput();
}

/*!
    \fn Kopete::AV::VideoDevicePool::currentInput()
 */
unsigned int VideoDevicePool::inputs()
{
    /// @todo implement me
	return m_videodevice[currentDevice()].inputs();
}

/*!
    \fn Kopete::AV::VideoDevicePool::loadSelectedDevice()
 */
void VideoDevicePool::loadSelectedDevice()
{
	kDebug() << "called";
	if (hasDevices() && (m_clients == 0))
	{
		KConfigGroup config(KGlobal::config(), "Video Device Settings");
		QString currentdevice = config.readEntry("Current Device", QString());
		kDebug() << "Current device: " << currentdevice;

//		m_current_device = 0; // Must check this thing because of the fact that multiple loadConfig in other methodas can do bad things. Watch out!

		VideoDeviceVector::iterator vditerator;
		for( vditerator = m_videodevice.begin(); vditerator != m_videodevice.end(); ++vditerator )
		{
			const QString modelindex = QString::fromLocal8Bit ( "Model %1 Device %2")  .arg ((*vditerator).m_name ) .arg ((*vditerator).m_modelindex);
			if(modelindex == currentdevice)
			{
				m_current_device = std::distance (m_videodevice.begin(), vditerator);
//				kDebug() << "This place will be used to set " << modelindex << " as the current device ( " << std::distance(m_videodevice.begin(), vditerator ) << " ).";
			}
			const QString name                = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Name")  .arg ((*vditerator).m_name ) .arg ((*vditerator).m_modelindex)), (*vditerator).m_model);
			kDebug() << "Device name: " << name;
		}
	}
}

/*!
    \fn Kopete::AV::VideoDevicePool::loadDeviceConfig()
 */
void VideoDevicePool::loadDeviceConfig()
{
    /// @todo implement me
	kDebug() << "called";
kDebug() << "WARNING: loading settings is currently deactivated !";
return;
	if (hasDevices() && (m_clients == 0))
	{
		KConfigGroup config(KGlobal::config(), "Video Device Settings");

		VideoDeviceVector::iterator vditerator;
		for( vditerator = m_videodevice.begin(); vditerator != m_videodevice.end(); ++vditerator )
		{
			const QString modelindex = QString::fromLocal8Bit ( "Model %1 Device %2")  .arg ((*vditerator).m_name ) .arg ((*vditerator).m_modelindex);
			const QString name                = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Name")  .arg ((*vditerator).m_name ) .arg ((*vditerator).m_modelindex)), (*vditerator).m_model);
			const int currentinput            = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Current input")  .arg ((*vditerator).m_name ) .arg ((*vditerator).m_modelindex)), 0);
			kDebug() << "Device name: " << name;
			kDebug() << "Device current input: " << currentinput;
			(*vditerator).selectInput(currentinput);

/*			for (int input = 0; input < (*vditerator).m_input.size(); input++)
			{
				const float brightness = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Brightness").arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , 0.5 );
				const float contrast   = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Contrast")  .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , 0.5 );
				const float saturation = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Saturation").arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , 0.5 );
				const float whiteness  = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Whiteness") .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , 0.5 );
				const float hue        = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Hue")       .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , 0.5 );
				const bool  autobrightnesscontrast = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 AutoBrightnessContrast") .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , false );
				const bool  autocolorcorrection    = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 AutoColorCorrection")    .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , false );
				const bool  imageasmirror          = config.readEntry((QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 ImageAsMirror")          .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input)) , false );
				(*vditerator).setBrightness(brightness);
				(*vditerator).setContrast(contrast);
				(*vditerator).setSaturation(saturation);
				(*vditerator).setWhiteness(whiteness);
				(*vditerator).setHue(hue);
				(*vditerator).setAutoBrightnessContrast(autobrightnesscontrast);
				(*vditerator).setAutoColorCorrection(autocolorcorrection);
				(*vditerator).setImageAsMirror(imageasmirror);
				kDebug() << "Brightness:" << brightness;
				kDebug() << "Contrast  :" << contrast;
				kDebug() << "Saturation:" << saturation;
				kDebug() << "Whiteness :" << whiteness;
				kDebug() << "Hue       :" << hue;
				kDebug() << "AutoBrightnessContrast:" << autobrightnesscontrast;
				kDebug() << "AutoColorCorrection   :" << autocolorcorrection;
				kDebug() << "ImageAsMirror         :" << imageasmirror;
			}*/
		}
	}
}

/*!
    \fn Kopete::AV::VideoDevicePool::saveConfig()
 */
void VideoDevicePool::saveConfig()
{
    /// @todo implement me
	kDebug() << "called";
kDebug() << "WARNING: saving settings is currently deactivated !";
return;
	if(hasDevices())
	{
		KConfigGroup config(KGlobal::config(), "Video Device Settings");

/*		if(m_modelvector.size())
		{
			VideoDeviceModelPool::m_devicemodel::iterator vmiterator;
			for( vmiterator = m_modelvector.begin(); vmiterator != m_modelvector.end(); ++vmiterator )
			{
				kDebug() << "Device Model: " << (*vmiterator).name;
				kDebug() << "Device Count: " << (*vmiterator).count;
			}
		}
*/
// Stores what is the current video device in use
		const QString currentdevice = QString::fromLocal8Bit ( "Model %1 Device %2" ) .arg(m_videodevice[m_current_device].m_model) .arg(m_videodevice[m_current_device].m_modelindex);
		config.writeEntry( "Current Device", currentdevice);

		VideoDeviceVector::iterator vditerator;
		for( vditerator = m_videodevice.begin(); vditerator != m_videodevice.end(); ++vditerator )
		{
			kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Name:" << (*vditerator).m_name;
			kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Current input:" << (*vditerator).currentInput();

// Stores current input for the given video device
			const QString name                   = QString::fromLocal8Bit ( "Model %1 Device %2 Name")  .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex);
			const QString currentinput           = QString::fromLocal8Bit ( "Model %1 Device %2 Current input")  .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex);
			config.writeEntry( name,                   (*vditerator).m_name);
			config.writeEntry( currentinput,           (*vditerator).currentInput());
/*
			for (int input = 0 ; input < (*vditerator).m_input.size(); input++)
			{
				kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Input:" << input << ":Brightness: " << (*vditerator).m_input[input].getBrightness();
				kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Input:" << input << ":Contrast  : " << (*vditerator).m_input[input].getContrast();
				kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Input:" << input << ":Saturation: " << (*vditerator).m_input[input].getSaturation();
				kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Input:" << input << ":Whiteness : " << (*vditerator).m_input[input].getWhiteness();
				kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Input:" << input << ":Hue       : " << (*vditerator).m_input[input].getHue();
				kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Input:" << input << ":Automatic brightness / contrast: " << (*vditerator).m_input[input].getAutoBrightnessContrast();
				kDebug() << "Model:" << (*vditerator).m_model << ":Index:" << (*vditerator).m_modelindex << ":Input:" << input << ":Automatic color correction     : " << (*vditerator).m_input[input].getAutoColorCorrection();

// Stores configuration about each channel
				const QString brightness             = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Brightness")             .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				const QString contrast               = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Contrast")               .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				const QString saturation             = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Saturation")             .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				const QString whiteness              = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Whiteness")              .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				const QString hue                    = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 Hue")                    .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				const QString autobrightnesscontrast = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 AutoBrightnessContrast") .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				const QString autocolorcorrection    = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 AutoColorCorrection")    .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				const QString imageasmirror          = QString::fromLocal8Bit ( "Model %1 Device %2 Input %3 ImageAsMirror")          .arg ((*vditerator).m_model ) .arg ((*vditerator).m_modelindex) .arg (input);
				config.writeEntry( brightness,             (double)(*vditerator).m_input[input].getBrightness());
				config.writeEntry( contrast,               (double)(*vditerator).m_input[input].getContrast());
				config.writeEntry( saturation,             (double)(*vditerator).m_input[input].getSaturation());
				config.writeEntry( whiteness,              (double)(*vditerator).m_input[input].getWhiteness());
				config.writeEntry( hue,                    (double)(*vditerator).m_input[input].getHue());
				config.writeEntry( autobrightnesscontrast, (*vditerator).m_input[input].getAutoBrightnessContrast());
				config.writeEntry( autocolorcorrection,    (*vditerator).m_input[input].getAutoColorCorrection());
				config.writeEntry( imageasmirror,          (*vditerator).m_input[input].getImageAsMirror());
			}*/
		}
		config.sync();
		kDebug();
	}
}

void VideoDevicePool::deviceAdded( const QString & udi )
{
	kDebug() << "("<< udi << ") called";
	Solid::Device dev( udi );
	if ( dev.is<Solid::Video>() )
	{
		registerDevice( dev );
		emit deviceRegistered( udi );
	}
}

void VideoDevicePool::deviceRemoved( const QString & udi )
{
	kDebug() << "("<< udi << ") called";
	int i = 0;
	m_ready.lock();
	foreach ( VideoDevice vd, m_videodevice )
	{
		if ( vd.udi() == udi )
		{
			kDebug() << "Video device '" << udi << "' has been removed!";
			// not sure if this is safe but at this point the device node is gone already anyway
			m_videodevice.remove( i );
			if (m_current_device == i)
			{
				m_current_device = 0;
				m_clients = 0;
			}
			else if (m_current_device > i)
			{
				m_current_device--;
			}
			m_ready.unlock();
			emit deviceUnregistered( udi );
			/* NOTE: do not emit deviceUnregistered( udi ) with mutex locked ! => potential deadlock ! */
			return;
		}
		else
		{
			i++;
		}
	}
	m_ready.unlock();
}

} // namespace AV

} // namespace Kopete
