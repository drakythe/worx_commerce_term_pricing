<?php

function worx_commerce_term_pricing_settings_form() {
  $temp_thing = array('BULK01');

  global $user;
  $order = commerce_cart_order_load($user->uid);
  foreach ($temp_thing as $sku) {
    $product = commerce_product_load_by_sku($sku);
    dsm($product);
  }
  foreach ($order->commerce_line_items[LANGUAGE_NONE] as $index => $array) {
    $line_item = commerce_line_item_load($array['line_item_id']);
    dsm($line_item);
  }


  $form['contact_information'] = array(
    '#markup' => t('You can leave us a message using the contact form below.'),
  );

  return $form;
}
