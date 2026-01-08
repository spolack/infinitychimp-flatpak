# Infinity Chimp 2.34 as flatpak

### Install

#### Build & Install flatpak
```
git clone git@github.com:spolack/infinitychimp-flatpak.git
cd infinitychimp-flatpak
flatpak-builder --install --user --install-deps-from=flathub --force-clean builddir/ com.highlite.infinitychimp.yaml
```

#### Add udev rules
```
echo 'SUBSYSTEM=="usb*", ACTION=="add|change", ATTRS{idVendor}=="0403", MODE="0666"' \
  | sudo tee /etc/udev/rules.d/90-chimp.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### Notes


#### Update Factory Library
Use the "Infinity Chimp Update Factory Library" shortcut in your app launcher to run the update script.

#### Data Storage
Files are stored at `~/Documents/Chimp`. This includes shows and the factory library. The folder is created automatically on start.

#### Networking & Art-Net: Workaround for interface selection
The app doesn't natively allow interface selection for Art-Net.

A workaround using LD_PRELOAD is inplace, which intercepts the sendto calls and redirects them to a interface provided BCAST_IFACE. If no environment variable is provided, the launcher script takes the interface with a IP in 2.0.0.0/8.

```
flatpak run --env=BCAST_IFACE=eth0 com.highlite.infinitychimp # Set for a single run
flatpak override --user --env=BCAST_IFACE=eth0 com.highlite.infinitychimp # Override permanently
```
