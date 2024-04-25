import astropy.io.fits as pyfits
import numpy as np

from astropy.coordinates import Angle

def update_header(header, ra_deg, dec_deg, crpix1, crpix2, pa, pixelscale):
    # ra_deg = Angle(ra, unit="hourangle").degree
    # dec_deg = Angle(dec, unit="degree").degree
    # pa = header["PASTART"] - 135

    #modify 20240312 by hilee
    
    '''
    h = dict(CTYPE1="RA---TAN",
             CTYPE2="DEC--TAN",
             CDELT1=-pixelscale,
             CDELT2=pixelscale,
             CRPIX1=crpix1,
             CRPIX2=crpix2,
             CRVAL1=ra_deg,
             CRVAL2=dec_deg,
             CROTA1=pa,
             CROTA2=pa)
    '''

    CDELT1=-pixelscale
    CDELT2=pixelscale
    CROTA1=pa
    CROTA2=pa

    h = dict(CTYPE1="RA---TAN",
             CTYPE2="DEC--TAN",
             # FR and Andy
             CRPIX1=crpix1,
             CRPIX2=crpix2,
             #CRPIX1=970,
             #CRPIX2=970,
             CRVAL1=ra_deg,
             CRVAL2=dec_deg,
             CD1_1=CDELT1*np.cos(np.deg2rad(CROTA2)),
             CD1_2=-CDELT2*np.sin(np.deg2rad(CROTA2)),
             CD2_1=CDELT1*np.sin(np.deg2rad(CROTA2)),
             CD2_2=CDELT2*np.cos(np.deg2rad(CROTA2)))

    for k, v in h.items():
        if isinstance(v, float) and np.isnan(v):
            v = pyfits.card.UNDEFINED
        header[k] = v

    return header


def update_header2(header, crpix1, crpix2, pa, pixelscale):
    #print(header)
    ra_deg = header["TELRA"]
    dec_deg = header["TELDEC"]

    #pa = header["PASTART"]

    #try:
        #ra_deg = Angle(ra, unit="hourangle").degree
        #dec_deg = Angle(dec, unit="degree").degree
    #except (TypeError, ValueError):
    #    ra_deg, dec_deg = np.nan, np.nan

    try:
        #pa = header["PASTART"] - 135
        pa = 225 - pa
    except TypeError:
        pa = np.nan

    header = update_header(header, ra_deg, dec_deg,
                           crpix1, crpix2, pa, pixelscale)

    return header


if __name__ == "__main__":

    f = pyfits.open("SDCS_20160921_0001.fits")
    header = f[0].header

    ra = header["TELRA"]
    dec = header["TELDEC"]

    pa = header["PA_START"]
    crpix1, crpix2 = 1024, 1024
    pixelscale = 0.074 / 3600.

    header = update_header(f[1].header, ra, dec, crpix1, crpix2,
                           pa, pixelscale)

    pyfits.PrimaryHDU(header=header, data=f[1].data).writeto("test.fits",
                                                             clobber=True)

