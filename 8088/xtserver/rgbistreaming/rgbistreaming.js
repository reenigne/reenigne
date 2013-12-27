var c=document.getElementById("myCanvas");
 var ctx=c.getContext("2d");
var imgData=ctx.createImageData(100,100);
for (var i=0;i<imgData.data.length;i+=4)
  {
  imgData.data[i+0]=255;   // red
  imgData.data[i+1]=0;     // green
  imgData.data[i+2]=0;     // blue
  imgData.data[i+3]=255;   // alpha
  }
 ctx.putImageData(imgData,10,10);





JavaScript syntax:
var imgData=context.createImageData(imageData); 

Parameter Values


Parameter

Description

width The width of the new ImageData object, in pixels 
height The height of the new ImageData object, in pixels 
imageData anotherImageData object 




JavaScript syntax:
context.putImageData(imgData,x,y,dirtyX,dirtyY,dirtyWidth,dirtyHeight);

Parameter Values


Parameter

Description

imgData Specifies the ImageData object to put back onto the canvas 
x The x-coordinate, in pixels, of the upper-left corner of the ImageData object 
y The y-coordinate, in pixels, of the upper-left corner of the ImageData object 
dirtyX Optional. The horizontal (x) value, in pixels, where to place the image on the canvas 
dirtyY Optional. The vertical (y) value, in pixels, where to place the image on the canvas 
dirtyWidth Optional. The width to use to draw the image on the canvas 
dirtyHeight Optional. The height to use to draw the image on the canvas 





  <?
    while (true) {
  ?>
      <script type="text/javascript">
        $('news').innerHTML = '<?= getLatestNews() ?>';
      </script>
  <?
      flush(); // Ensure the Javascript tag is written out immediately
      sleep(10);
    }
  ?>



