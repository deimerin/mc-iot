const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');
const vision = require('@google-cloud/vision');

const app = express();
const port = process.env.PORT || 8888;
const validNames = ['wood', 'darkness', 'fur'];

app.use(bodyParser.raw({ type: 'application/octet-stream' }));
app.use(express.static(path.join(__dirname, 'public')));
app.use('/image', express.static(path.join(__dirname, 'resources')));

async function labelAPI() {
  try {
    const client = new vision.ImageAnnotatorClient();
    const [result] = await client.labelDetection('./resources/image.jpg');
    const labels = result.labelAnnotations.map(label => ({
      name: label.description,
      score: label.score
    }));
    return labels;
  } catch (error) {
    console.error('Error in labelAPI:', error);
    throw error;
  }
}

async function objectAPI() {
  try {
    const client = new vision.ImageAnnotatorClient();
    const [result] = await client.objectLocalization('./resources/image.jpg');
    const objects = result.localizedObjectAnnotations.map(object => ({
      name: object.name,
      score: object.score
    }));
    return objects;
  } catch (error) {
    console.error('Error in objectAPI:', error);
    throw error;
  }
}

function isValidName(name){
  const lowerCaseName = name.toLowerCase();
  return validNames.map(validName => validName.toLowerCase()).includes(lowerCaseName);
}

app.post('/upload', async (req, res) => {
  // Save the received image to a file (overwrite the old image)
  fs.writeFileSync('./resources/image.jpg', req.body);
  console.log('Image saved successfully');

  const objects = await objectAPI();
  const labels = await labelAPI();

  fs.writeFileSync('./resources/objects.txt', JSON.stringify(objects, null, 2));
  fs.writeFileSync('./resources/labels.txt', JSON.stringify(labels, null, 2));

  const filteredData = [];

  objects.forEach(object => {
    if (isValidName(object.name)) {
      filteredData.push({
        type: 'object',
        name: object.name,
        score: object.score,
      });
    }
  });

  labels.forEach(label => {
    if (isValidName(label.name)) {
      filteredData.push({
        type: 'label',
        name: label.name,
        score: label.score,
      });
    }
  });

  const responseJSON = {
    filteredData,
  };

  console.log(responseJSON);

  res.status(200).json(responseJSON);
});

app.post('/updateValidNames', express.json(), (req, res) => {

  const newValidNames = req.body.validNames || [];
  validNames.length = 0;
  validNames.push(...newValidNames);

  console.log(validNames);

  res.status(200).json({ message: 'Valid names updated successfully' });
});

app.get('/objects', (req, res) => {
  const objects = JSON.parse(fs.readFileSync('./resources/objects.txt', 'utf-8'));
  res.json(objects);
});

app.get('/labels', (req, res) => {
  const labels = JSON.parse(fs.readFileSync('./resources/labels.txt', 'utf-8'));
  res.json(labels);
});

app.get('/update', (req, res) => {
  res.sendFile(path.join(__dirname, 'updateValidNames.html'));
});

app.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});