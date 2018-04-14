<h5><a href="http://bensoft.de/projects/smartmirror">Visit my webpage! (images etc.)</a></h5>
<h1>SmartMirror</h1>
						<h2>Description</h2>
						<p>
							SmartMirror is a DIY project. Basically it is an LED-Matrix of 64x16 placed behind a translucent mirror. It is powered by a NodeMCU chip, so it has WiFi capabilities. It can show the time in some special decimal form (where the day has 10 hours, a hour 100 minutes and a minute 100 seconds). A DHT11 temperature and humidity sensor provides ambient values. Through implementation of a simple HTTP server, the mirror can react to GET requests. It gathers the current time via NTP (thus it needs to be connected to the Internet).
						</p>
						<p>
							Required libraries:<br>
							<ul>
								<li><a href="https://github.com/Seeed-Studio/Ultrathin_LED_Matrix">Ultrathin_LED_Matrix by Seeed-Studio</a></li>
								<li><a href="https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib">DHTLib by RobTillaart</a></li>
								<li><a href="https://github.com/PaulStoffregen/Time">Time by PaulStoffregen</a></li>
								<li><a href="https://github.com/JChristensen/Timezone">Timezone by JChristensen</a></li>
							</ul>
						</p>
						<h2>GET request modes</h2>
						<p>
							<table class="bordered">
								<tr><td>/clock</td><td>Shows the clock.</td></tr>
								<tr><td>/temperature</td><td>Returns the temperature.</td></tr>
								<tr><td>/humidity</td><td>Returns the humidity.</td></tr>
								<tr><td>/disable</td><td>Shuts off the display (but continues to respond to requests).</td></tr>
								<tr><td>/enable</td><td>Turns the display on.</td></tr>
								<tr><td>/text?</td><td>Shows text on the screen.<br>
														style=[ marquee | rightbound | leftbound | center ], default is marquee<br>
														In case of marquee: direction=[ right | left ], default is left; speed=THE HIGHER, THE SLOWER<br></td></tr>
							</table>
						</p>
						<h2>Parts list</h2>
						<p>
							<ul>
								<li>
									A mirror frame (thick) with glass
								</li>
								<li>
									<a href="http://www.ebay.de/itm/33-33-EUR-pro-m-Sonnenschutz-Spiegel-Fensterfolie-UV-Sichtschutz-Gebaudefolie-/201609124889?var=&hash=item0">Translucent glass foil</a> (2€)
								</li>
								<li>
									<a href="http://www.ebay.de/itm/NodeMCU-V3-ESP8266-ESP-12-E-Lua-CH340-WiFI-WLan-IoT-32-Bit-microUSB-Arduino-/162472635911?hash=item25d41fae07">NodeMCU chip</a> (5-6€)
								</li>
								<li>
									<a href="http://www.ebay.de/itm/DHT11-Digital-Luftfeuchtigkeit-Temperatursensor-Modul-Arduino-Pi-Atmel-PIC-/222638469234?hash=item33d6497072">DHT11 sensor</a> (3€)
								</li>
								<li>
									<a href="http://www.ebay.de/itm/191793765829">LED Matrix</a> (14€)
								</li>
								<li>
									Several cables (jumper wires | USB cables, splitter, on/off-switch)
								</li>
							</ul>
						</p>
