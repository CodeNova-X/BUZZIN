

# BUZZIN
<img width="1500" height="500" alt="BUZZIN Banner" src="https://github.com/user-attachments/assets/f6545d9f-6f22-46fc-81bb-5ba67f7f5581" />

## Intro-

  BUZZIN is a custom-built wireless game buzzer which I designed to bring the excitement to games! I really wanted to make a game buzzer that can do more than what others have made and able to be modified easier than others. So designed my own that can be used wirelessly and rechargeable and programmable. The full system includes multiple controllers and 1 hub, both of them use the same parts and same PCB to make it very simple for someone else to make themselves. 
  
  The controllers and the hub are powered by a LiPo battery which can be recharged with a usb c and uses a Xiao Esp32 C5 as the brain to control it. I have designed the PCB in order that those who want to can design a more complex buzzer and not need to redesign the PCB. The PCB allows for extra buttons and the ability to connect an LED strip to use instead of a single LED. With this possibility to expand your buzzer allows for you to adapt my initial design and use it for more specific games. It has a minimal footprint so it can be used in either small or large projects.

  My hope is that Hack Club will be able to fund this project. I designed the schematic and PCB in EasyEDA-Pro and the designs for the controller and the hub were designed in Fusion 360. None of this is tested so I cannot confirm if everything will work but I will be updating this project when I finally have all the parts.
  

  
> ### Schematic and PCB V1
> V1. Incorporates a led and vibration motor for feedback you can feel. [Old version]
>
> This is V1 of BUZZIN, I decided to change it in the end however because it was bigger than it needed to be and I decided that V1 could be made cheaper.
> 
> *Technically this was not the first version and I actually had a previous version which used an 18650 battery but I decided to not use it.
>
> <img height="300" alt="3" src="https://github.com/user-attachments/assets/679a8c8b-6777-4ef7-bd75-52f758a8fd5a" /><img height="300" alt="4" src="https://github.com/user-attachments/assets/b4009225-2fb7-4c6a-a412-1efab0a25d2e" /><img height="300" alt="5" src="https://github.com/user-attachments/assets/de629f28-2326-41ba-aa5f-2bd70c85b978" /> <img width="400" alt="2" src="https://github.com/user-attachments/assets/45da131b-922f-4a72-9127-7a3fc9a0df43" />



## Schematic and PCB V1.5
  V1.5 Smaller and more compact. Instead using a vibrating motor, I changed to a small buzzer instead to cut the cost and space a bit. It includes extra pins if you would like to expand the design further without having to redesign the pcb. With these extra pins, you can expand this system up to 5 buttons and can attach an LED strip or equivalent instead of a single led. Note- the led strip expansion and the led both use pin D4. With this expansion, I thought it necessary to add screw holes if needed for other designs, you can use 2 M3 screws to attach the PCB to other things. In order to make the design smaller, I also removed the switch and instead the controllers would just use deep sleep mode.

<img height="300" height="428" alt="Screenshot 2026-10-31 at 6 18 48 PM" src="https://github.com/user-attachments/assets/ba210c77-9848-44fc-b648-83786e7e01a8" />
<img height="300" alt="Screenshot 2026-10-31 at 7 01 55 PM" src="https://github.com/user-attachments/assets/dd732265-1f99-4aab-a11b-a98ee8c15ec2" />
<img height="300" alt="Screenshot 2026-10-31 at 7 02 07 PM" src="https://github.com/user-attachments/assets/8fa64ce2-1424-4429-852e-a02517fe0b4d" />
<img height="300" alt="Screenshot 2026-10-31 at 2 19 21 PM" src="https://github.com/user-attachments/assets/23876aa4-ed12-4056-a85e-a850c0de1fdf" />


## BUZZIN controller
  I made the controller to be handheld and and use as little space as possible with my PCB design. Of course though it does need to look like this and can be incorporated into many different designs. It just uses one button to be able to buzz in and to wake it up from deep sleep. I designed it so that it needs no screws and can be put together friction but it may not work and be different for different filaments so you might need glue.
  
<img height="300" alt="Screenshot 2026-10-31 at 3 45 37 PM" src="https://github.com/user-attachments/assets/f1877c9a-6d0f-4755-85c4-dfac90ee61d5" />

<img height="300" alt="Screenshot 2026-10-31 at 5 06 17 PM" src="https://github.com/user-attachments/assets/fecc5f65-356a-49bb-b2b9-5df89e9676d3" />


## BUZZIN HUB 
  I designed the hub around the same pcb as the controller and could technically be used as a controller. It can be used with or without a battery if you would rather just plug it in. Same with the controller, there is no switch and will use deep sleep mode. The idea is that the led will show through the white filament to be able to show different states whether it be a winning state for a controller or whatever really. The design is is made so that it does not need screws and consist of an inner and outer shells. The inner shell is open on one side to allow for the battery and the PCB. The outer shells go around the inner shell to keep everything inside.

https://github.com/user-attachments/assets/d7611ec3-7f72-404c-8847-28c70f6caadd

## BOM
### BOM for BUZZIN (4 Controllers & 1 Hub)

| Component | Part / Model | Qty. Needed | Price per Part | Total | Status | Supplier Link | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Microcontroller | Seeed XIAO ESP32-C5 | 5 | $6.99 | $34.95 | Purchase | [Seeed Studio](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C5-p-6609.html) | |
| Buzzer | Huaneng MLT-8530 (C94599) | 5 | $0.2038 | $1.02 | Owned | [LCSC](https://www.lcsc.com/product-detail/C94599.html?globalKeyword=C94599&s_z=n_q_C94599) | |
| RGB LED | WS2812B-MINI-V6 (C52941386) | 5 | $0.1079 | $0.54 | Owned | [LCSC](https://www.lcsc.com/product-detail/C52941386.html?globalKeyword=C52941386&s_z=n_q_C52941386) | |
| Battery PCB connector | Molex 53261-0271 (C177225) | 5 | $0.1433 | $0.72 | Owned | [LCSC](https://www.lcsc.com/product-detail/C177225.html?globalKeyword=C177225&s_z=n_q_C177225) | |
| External pushbutton | Gebildet B0CJHXJWN3 | 5 | $1.398* | $6.99 | Purchase | [Amazon](https://www.amazon.com/dp/B0CJHXJWN3?th=1) | |
| LiPo battery | YDL 102050, 3.7V 1000mAh | 5 | $2.10 | $10.50 | Purchase | [YDL Battery](https://ydlbattery.com/products/3-7v-1000mah-102050-lithium-polymer-ion-battery?variant=42093651787929) | Make sure to choose the Molex 1.25 connector and the price is discounted for buying 10+ (I am getting 5 for project, 5 for personal use) |
| Custom PCB | PCBWay | 5 | $0.42 | $2.10 | Purchase | [JLCPCB](https://jlcpcb.com) | Price discount via JLCONE app |

### Shipping, Taxes, and Fees

| Shipping / Fee | Cost | Details |
| :--- | :--- | :--- |
| XIAO shipping + sales tax | $16.97 | Actual checkout amount |
| Battery shipping | $7.85 | Full shipping for 10 batteries |
| PCB shipping | $3.12 | Already included in $5.22 PCB quote |
| Estimated PCB import / processing fee | $10.00 | Provisional estimate, not confirmed |
| LCSC shipping for owned parts | Unknown | Exact amount unknown; parts already on hand |
| Amazon button shipping | $0.00 | Free shipping with Prime |
| Amazon button tax | $4.00 | Rough tax estimate on button |

### Final Pricing

| Category | Cost (for each category) | Details |
| :--- | :--- | :--- |
| New components and PCB quote | $54.54 | |
| Shipping | $31.94 | Also includes taxes |
| Estimated PCB import / processing fee | $10.00 | |
| Total | **$96.48** |

